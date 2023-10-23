// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"
#include "sway.h"

#include <stdbool.h>
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

// Identifiers of the last focused window and its tab
static uint32_t last_wnd;
static uint32_t last_tab;
// Default layout for new windows
static int default_layout = DEFAULT_LAYOUT;
// Currently active layout
static int current_layout = INVALID_LAYOUT;
// Ignored time between layout change and focus lost
static size_t switch_timeout = DEFAULT_TIMEOUT;
static struct timespec switch_timestamp;
// List of tab-enbled applications
static char** tab_apps_list;
static size_t tab_apps_num;
// Verbose (event trace) mode
static bool verbose = false;
#define TRACE(fmt, ...) if (verbose) printf("%s: " fmt "\n", __func__, ##__VA_ARGS__)

/** Focus change handler. */
static int on_focus_change(int wnd_id, const char* app_id, const char* title)
{
    int layout;
    uint32_t tab_id = 0;

    // generate unique tab id from window title (if it is a browser)
    if (app_id && title) {
        for (size_t i = 0; i < tab_apps_num; ++i) {
            if (strcmp(app_id, tab_apps_list[i]) == 0) {
                const char* ptr = title;
                // djb2 hash
                tab_id = 5381;
                while (*ptr) {
                    tab_id = ((tab_id << 5) + tab_id) + *ptr++;
                }
                break;
            }
        }
    }

    // save current layout for previously focused window
    if (last_wnd && current_layout != INVALID_LAYOUT) {
        if (switch_timeout == 0) {
            layout = current_layout;
        } else {
            // check for timeout
            size_t elapsed;
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            elapsed = TIMESPEC_MS(ts) - TIMESPEC_MS(switch_timestamp);
            if (elapsed > switch_timeout) {
                layout = current_layout;
            } else {
                layout = INVALID_LAYOUT;
            }
        }
        if (layout != INVALID_LAYOUT) {
            TRACE("store layout=%d, window=%x:%x", layout, last_wnd, last_tab);
            put_layout(last_wnd, last_tab, layout);
        }
    }

    // define layout for currently focused window
    layout = get_layout(wnd_id, tab_id);
    TRACE("found layout=%d, window=%x:%x", layout, wnd_id, tab_id);
    if (layout == INVALID_LAYOUT && default_layout != INVALID_LAYOUT) {
        layout = default_layout; // set default
    }
    if (layout == current_layout) {
        layout = INVALID_LAYOUT; // already set
    }

    last_wnd = wnd_id;
    last_tab = tab_id;

    TRACE("set layout=%d, window=%x:%x", layout, wnd_id, tab_id);
    return layout;
}

/** Title change handler. */
static int on_title_change(int wnd_id, const char* app_id, const char* title)
{
    if (last_wnd == (uint32_t)wnd_id) {
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

    if (last_wnd == (uint32_t)wnd_id) {
        // reset last window id to prevent saving layout for the closed window
        last_wnd = 0;
    }
}

/** Keyboard layout change handler. */
static void on_layout_change(int layout)
{
    TRACE("layout=%d, window=%x:%x", layout, last_wnd, last_tab);
    current_layout = layout;
    clock_gettime(CLOCK_MONOTONIC, &switch_timestamp);
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
        { "verbose", no_argument,       NULL, 'V' },
        { "version", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      0,                 NULL,  0  }
    };
    const char* short_opts = "d:t:a:Vvh";
    const char* tab_apps = DEFAULT_TABAPPS;

    opterr = 0; // prevent native error messages

    // parse arguments
    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd':
                default_layout = atoi(optarg);
                if (default_layout < -1 || default_layout > 0xffff) {
                    fprintf(stderr, "Invalid default layout: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                switch_timeout = atoi(optarg);
                break;
            case 'a':
                tab_apps = optarg;
                break;
            case 'V':
                verbose = true;
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
                printf("  -V, --verbose     Enable verbose output (event trace)\n");
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

    if (*tab_apps) {
        size_t i = 0;
        // create list of tab-enabled app IDs
        tab_apps_num = 1;
        for (const char* ptr = tab_apps; *ptr; ++ptr) {
            if (*ptr == ',') {
                ++tab_apps_num;
            }
        }
        tab_apps_list = malloc(tab_apps_num * sizeof(char*));
        // split into array
        for (const char* ptr = tab_apps;; ++ptr) {
            if (!*ptr || *ptr == ',') {
                const size_t len = ptr - tab_apps;
                char* app_id = malloc(len + 1 /* last null */);
                memcpy(app_id, tab_apps, len);
                app_id[len] = 0;
                tab_apps_list[i++] = app_id;
                tab_apps = ptr + 1;
                if (!*ptr) {
                    break;
                }
            }
        }
    }

    return sway_monitor(on_focus_change, on_title_change,
                        on_window_close, on_layout_change);
}
