// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "sway.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <json.h>

/** IPC magic header value */
static const uint8_t ipc_magic[] = { 'i', '3', '-', 'i', 'p', 'c' };

/** IPC message types (used only) */
enum ipc_msg_type {
    IPC_COMMAND = 0,
    IPC_SUBSCRIBE = 2,
};

/** IPC header */
struct __attribute__((__packed__)) ipc_header {
    uint8_t magic[sizeof(ipc_magic)];
    uint32_t len;
    uint32_t type;
};

/**
 * Read exactly specified number of bytes from socket.
 * @param[in] sock socket descriptor
 * @param[out] buf buffer for destination data
 * @param[in] len number of bytes to read
 * @return error code, 0 on success
 */
static int sock_read(int sock, void* buf, size_t len)
{
    while (len) {
        const ssize_t rcv = recv(sock, buf, len, 0);
        if (rcv == 0) {
            fprintf(stderr, "IPC read error: no data\n");
            return ENOMSG;
        }
        if (rcv == -1) {
            const int ec = errno;
            fprintf(stderr, "IPC read error: [%i] %s\n", ec, strerror(ec));
            return ec;
        }
        len -= rcv;
        buf = ((uint8_t*)buf) + rcv;
    }
    return 0;
}

/**
 * Write data to the socket.
 * @param[in] sock socket descriptor
 * @param[in] buf buffer of data of send
 * @param[in] len number of bytes to write
 * @return error code, 0 on success
 */
static int sock_write(int sock, const void* buf, size_t len)
{
    while (len) {
        const ssize_t rcv = write(sock, buf, len);
        if (rcv == -1) {
            const int ec = errno;
            fprintf(stderr, "IPC write error: [%i] %s\n", ec, strerror(ec));
            return ec;
        }
        len -= rcv;
        buf = ((uint8_t*)buf) + rcv;
    }
    return 0;
}

/**
 * Read IPC message.
 * @param[in] sock socket descriptor
 * @return IPC response as json object, NULL on errors
 */
static struct json_object* ipc_read(int sock)
{
    struct ipc_header hdr;
    if (sock_read(sock, &hdr, sizeof(hdr))) {
        return NULL;
    }
    char* raw = malloc(hdr.len + 1);
    if (!raw) {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    if (sock_read(sock, raw, hdr.len)) {
        free(raw);
        return NULL;
    }
    raw[hdr.len] = 0;

    struct json_object* response = json_tokener_parse(raw);
    if (!response) {
        fprintf(stderr, "Invalid IPC response\n");
    }

    free(raw);

    return response;
}

/**
 * Write IPC message.
 * @param[in] sock socket descriptor
 * @param[in] type message type
 * @param[in] payload payload data
 * @return error code, 0 on success
 */
static int ipc_write(int sock, enum ipc_msg_type type, const char* payload)
{
    struct ipc_header hdr;
    memcpy(hdr.magic, ipc_magic, sizeof(ipc_magic));
    hdr.len = payload ? strlen(payload) : 0;
    hdr.type = type;

    int rc = sock_write(sock, &hdr, sizeof(hdr));
    if (rc == 0 && hdr.len) {
        rc = sock_write(sock, payload, hdr.len);
    }

    return rc;
}

/**
 * Connect to Sway IPC.
 * @return socket descriptor (non-negative number) on successful completion,
 *         otherwise error code (negative number)
 */
static int ipc_connect(void)
{
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));

    const char* path = getenv("SWAYSOCK");
    if (!path) {
        fprintf(stderr, "SWAYSOCK variable is not defined\n");
        return -ENOENT;
    }
    size_t len = strlen(path);
    if (!len || len > sizeof(sa.sun_path)) {
        fprintf(stderr, "Invalid SWAYSOCK variable\n");
        return -ENOENT;
    }

    const int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        const int ec = errno;
        fprintf(stderr, "Failed to create IPC socket: [%i] %s\n", ec, strerror(ec));
        return -ec;
    }

    sa.sun_family = AF_UNIX;
    memcpy(sa.sun_path, path, len);

    len += sizeof(sa) - sizeof(sa.sun_path);
    if (connect(sock, (struct sockaddr*)&sa, len) == -1) {
        const int ec = errno;
        fprintf(stderr, "Failed to connect IPC socket: [%i] %s\n", ec, strerror(ec));
        close(sock);
        return -ec;
    }

    return sock;
}

/**
 * Subscribe to Sway events.
 * @param[in] sock socket descriptor
 * @return error code, 0 on success
 */
static int ipc_subscribe(int sock)
{
    const char* subscribe = "[ \"window\", \"input\" ]";
    int rc = ipc_write(sock, IPC_SUBSCRIBE, subscribe);
    if (rc == 0) {
        struct json_object* response = ipc_read(sock);
        if (!response) {
            rc = EIO;
        } else {
            struct json_object* val;
            if (!json_object_object_get_ex(response, "success", &val) ||
                !json_object_get_boolean(val)) {
                fprintf(stderr, "Unable to subscribe\n");
                rc = EIO;
            }
            json_object_put(response);
        }
    }
    return rc;
}

/**
 * Set keyboard layout.
 * @param[in] sock socket descriptor
 * @param[in] layout keyboard layout index to set
 * @return error code, 0 on success
 */
static int ipc_change_layout(int sock, int layout)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "input * xkb_switch_layout %i", layout);
    return ipc_write(sock, IPC_COMMAND, cmd);
}

/**
 * Get container info from event message.
 * @param[in] msg event message
 * @param[out] wnd_id identifier of currently focused window (container)
 * @param[out] app_id application id
 * @param[out] title title of the window
 */
static void container_info(struct json_object* msg, int* wnd_id,
                           const char** app_id, const char** title)
{
    struct json_object* cnt_node;
    struct json_object* sub_node;

    if (!json_object_object_get_ex(msg, "container", &cnt_node)) {
        return;
    }
    if (wnd_id && json_object_object_get_ex(cnt_node, "id", &sub_node)) {
        *wnd_id = json_object_get_int(sub_node);
    }
    if (app_id && json_object_object_get_ex(cnt_node, "app_id", &sub_node)) {
        *app_id = json_object_get_string(sub_node);
    }
    if (title && json_object_object_get_ex(cnt_node, "name", &sub_node)) {
        *title = json_object_get_string(sub_node);
    }
}

/**
 * Get keyboard layout index from event message.
 * @param[in] msg event message
 * @return keyboard layout index or -1 if not found
 */
static int layout_index(struct json_object* msg)
{
    struct json_object* input_node;
    if (json_object_object_get_ex(msg, "input", &input_node)) {
        struct json_object* index_node;
        if (json_object_object_get_ex(input_node, "xkb_active_layout_index",
                                      &index_node)) {
            const int idx = json_object_get_int(index_node);
            if (idx != 0 || errno != EINVAL) {
                return idx;
            }
        }
    }
    return -1;
}

int sway_monitor(on_focus fn_focus, on_title fn_title,
                 on_close fn_close, on_layout fn_layout)
{
    int rc;

    const int sock = ipc_connect();
    if (sock < 0) {
        rc = -sock;
        goto error;
    }

    rc = ipc_subscribe(sock);
    if (rc) {
        goto error;
    }

    while (rc == 0) {
        struct json_object* msg = ipc_read(sock);
        if (!msg) {
            rc = EIO;
        } else {
            struct json_object* event_node;
            if (json_object_object_get_ex(msg, "change", &event_node)) {
                int wnd_id = -1;
                int layout = -1;
                const char* app_id = NULL;
                const char* title = NULL;
                const char* event_name = json_object_get_string(event_node);
                if (strcmp(event_name, "focus") == 0) {
                    container_info(msg, &wnd_id, &app_id, &title);
                    layout = fn_focus(wnd_id, app_id, title);
                } else if (strcmp(event_name, "title") == 0) {
                    container_info(msg, &wnd_id, &app_id, &title);
                    layout = fn_title(wnd_id, app_id, title);
                } else if (strcmp(event_name, "close") == 0) {
                    container_info(msg, &wnd_id, NULL, NULL);
                    layout = fn_close(wnd_id);
                } else if (strcmp(event_name, "xkb_layout") == 0) {
                    layout = layout_index(msg);
                    if (layout >= 0) {
                        fn_layout(layout);
                    }
                    layout = -1;
                }
                if (layout >= 0) {
                    ipc_change_layout(sock, layout);
                }
            }
            json_object_put(msg);
        }
    }

error:
    if (sock >= 0) {
        close(sock);
    }
    return rc;
}
