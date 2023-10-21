// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"
#include "sway.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Default layout for new windows
#define DEFAULT_LAYOUT  0
// Default ignored time between layout change and focus lost events
#define DEFAULT_TIMEOUT 50
// Default list of tab-enabled app IDs
#define DEFAULT_TABAPPS "firefox,chrome"

// Convert timespec to milliseconds
#define TIMESPEC_MS(ts) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000)

// Debug trace
#if 0
#define TRACE(fmt, ...) printf("%s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define TRACE(fmt, ...)
#endif

/** Static context. */
struct context {
    uint32_t last_wnd;                ///< Identifier of the last focused window
    uint32_t last_tab;                ///< Identifier of the last focused tab
    int default_layout;               ///< Default layout for new windows
    int current_layout;               ///< Current layout index
    size_t switch_timeout;            ///< Ignored time between layout change and focus lost
    struct timespec switch_timestamp; ///< Timestamp of the last layout change
    char** tabapps;                   ///< List of tab-enbled applications
    size_t tabapps_num;               ///< Size of tabbaps list
};
static struct context ctx = {
    .last_wnd = 0,
    .last_tab = 0,
    .default_layout = DEFAULT_LAYOUT,
    .current_layout = INVALID_LAYOUT,
    .switch_timeout = DEFAULT_TIMEOUT,
};

/**
 * Generate unique tab id for tab-enabled applications.
 * @param[in] app_id application id
 * @param[in] title title of the window
 * @return tab id or 0 if application does not support tabs
 */
static uint32_t get_tab_id(const char* app_id, const char* title)
{
    for (size_t i = 0; i < ctx.tabapps_num; ++i) {
        if (strcmp(app_id, ctx.tabapps[i]) == 0) {
            // djb2 hash
            uint32_t hash = 5381;
            while (*title) {
                hash = ((hash << 5) + hash) + *title++;
            }
            return hash;
        }
    }
    return 0;
}

/** Focus change handler. */
static int on_focus_change(int wnd_id, const char* app_id, const char* title)
{
    int layout;
    const uint32_t tab_id = get_tab_id(app_id, title);

    // save current layout for previously focused window
    if (ctx.last_wnd && ctx.current_layout != INVALID_LAYOUT) {
        if (ctx.switch_timeout == 0) {
            layout = ctx.current_layout;
        } else {
            // check for timeout
            size_t elapsed;
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            elapsed = TIMESPEC_MS(ts) - TIMESPEC_MS(ctx.switch_timestamp);
            if (elapsed > ctx.switch_timeout) {
                layout = ctx.current_layout;
            } else {
                layout = INVALID_LAYOUT;
            }
        }
        if (layout != INVALID_LAYOUT) {
            TRACE("store layout=%d, window=%x:%x", layout, ctx.last_wnd, ctx.last_tab);
            put_layout(ctx.last_wnd, ctx.last_tab, layout);
        }
    }

    // define layout for currently focused window
    layout = get_layout(wnd_id, tab_id);
    TRACE("found layout=%d, window=%x:%x", layout, wnd_id, tab_id);
    if (layout == INVALID_LAYOUT && ctx.default_layout != INVALID_LAYOUT) {
        layout = ctx.default_layout; // set default
    }
    if (layout == ctx.current_layout) {
        layout = INVALID_LAYOUT; // already set
    }

    ctx.last_wnd = wnd_id;
    ctx.last_tab = tab_id;

    TRACE("set layout=%d, window=%x:%x", layout, wnd_id, tab_id);
    return layout;
}

/** Title change handler. */
static int on_title_change(int wnd_id, const char* app_id, const char* title)
{
    if (ctx.last_wnd == (uint32_t)wnd_id) {
        TRACE("window_id=%d", wnd_id);
        return on_focus_change(wnd_id, app_id, title);
    }
    return INVALID_LAYOUT;
}

/** Window close handler. */
static void on_window_close(int wnd_id)
{
    TRACE("window=%x:*", wnd_id);
    rm_layout(wnd_id);

    if (ctx.last_wnd == (uint32_t)wnd_id) {
        // reset last window id to prevent saving layout for the closed window
        ctx.last_wnd = 0;
    }
}

/** Keyboard layout change handler. */
static void on_layout_change(int layout)
{
    TRACE("layout=%d, window=%x:%x", layout, ctx.last_wnd, ctx.last_tab);
    ctx.current_layout = layout;
    clock_gettime(CLOCK_MONOTONIC, &ctx.switch_timestamp);
}

/** Set list of tab-enabled app IDs from command line arument. */
static void set_tabapps(const char* ids)
{
    size_t size;

    // get number of app ids
    size = 1;
    for (const char* ptr = ids; *ptr; ++ptr) {
        if (*ptr == ',') {
            ++size;
        }
    }

    // split into array
    ctx.tabapps = malloc(size * sizeof(char*));
    for (const char* ptr = ids;; ++ptr) {
        if (!*ptr || *ptr == ',') {
            const size_t len = ptr - ids;
            char* id = malloc(len + 1 /* last null */);
            memcpy(id, ids, len);
            id[len] = 0;
            ids = ptr + 1;
            ctx.tabapps[ctx.tabapps_num++] = id;
            if (!*ptr) {
                break;
            }
        }
    }
}

/**
 * Application entry point.
 */
int main(int argc, char* argv[])
{
    const struct option long_opts[] = {
        { "default", required_argument, NULL, 'd' },
        { "timeout", required_argument, NULL, 't' },
        { "tabapps", required_argument, NULL, 'a' },
        { "version", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      0,                 NULL,  0  }
    };
    const char* short_opts = "d:t:a:vh";

    opterr = 0; // prevent native error messages

    // parse arguments
    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd':
                ctx.default_layout = atoi(optarg);
                if (ctx.default_layout < -1 || ctx.default_layout > 0xffff) {
                    fprintf(stderr, "Invalid default layout: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                ctx.switch_timeout = atoi(optarg);
                break;
            case 'a':
                set_tabapps(optarg);
                break;
            case 'v':
                printf("swaykbdd version " VERSION ".\n");
                return EXIT_SUCCESS;
            case 'h':
                printf("Keyboard layout switcher for Sway.\n");
                printf("Usage: %s [OPTION]\n", argv[0]);
                printf("  -d, --default=ID  Default layout for new windows "
                       "[%i]\n", DEFAULT_LAYOUT);
                printf("  -t, --timeout=MS  Delay between switching and "
                       "saving layout [%i ms]\n", DEFAULT_TIMEOUT);
                printf("  -a, --tabapps=IDS List of tab-enabled app IDs "
                       "[" DEFAULT_TABAPPS "]\n");
                printf("  -v, --version     Print version info and exit\n");
                printf("  -h, --help        Print this help and exit\n");
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Invalid argument: %s\n", argv[optind - 1]);
                return EXIT_FAILURE;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    if (ctx.tabapps_num == 0) {
        set_tabapps(DEFAULT_TABAPPS);
    }

    return sway_monitor(on_focus_change, on_title_change,
                        on_window_close, on_layout_change);
}
