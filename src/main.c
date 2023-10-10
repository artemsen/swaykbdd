// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"
#include "sway.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Default layout for new windows
#define DEFAULT_LAYOUT  0
// Default ignored time between layout change and focus lost events
#define DEFAULT_TIMEOUT 50

// Convert timespec to milliseconds
#define TIMESPEC_MS(ts) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000)

/** Static context. */
struct context {
    unsigned long last_window;        ///< Identifier of the last focused window
    int default_layout;               ///< Default layout for new windows
    int current_layout;               ///< Current layout index
    int switch_timeout;               ///< Ignored time between layout change and focus lost
    struct timespec switch_timestamp; ///< Timestamp of the last layout change
};
static struct context ctx = {
    .last_window = INVALID_WINDOW,
    .default_layout = DEFAULT_LAYOUT,
    .current_layout = INVALID_LAYOUT,
    .switch_timeout = DEFAULT_TIMEOUT,
};

/** Focus change handler. */
static int on_focus_change(unsigned long window)
{
    int layout;

    // save current layout for previously focused window
    if (ctx.last_window != INVALID_WINDOW &&
        ctx.current_layout != INVALID_LAYOUT) {
        if (ctx.switch_timeout == 0) {
            layout = ctx.current_layout;
        } else {
            // check for timeout
            unsigned long long elapsed;
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            elapsed = TIMESPEC_MS(ts) - TIMESPEC_MS(ctx.switch_timestamp);
            if (elapsed > (unsigned long long)ctx.switch_timeout) {
                layout = ctx.current_layout;
            } else {
                layout = INVALID_LAYOUT;
            }
        }
        if (layout != INVALID_LAYOUT) {
            put_layout(ctx.last_window, layout);
        }
    }

    // define layout for currently focused window
    layout = get_layout(window);
    if (layout == INVALID_LAYOUT && ctx.default_layout != INVALID_LAYOUT) {
        layout = ctx.default_layout; // set default
    }
    if (layout == ctx.current_layout) {
        layout = INVALID_LAYOUT; // already set
    }

    ctx.last_window = window;

    return layout;
}

/** Window close handler. */
static void on_window_close(unsigned long window)
{
    rm_layout(window);
    if (window == ctx.last_window) {
        // reset last window id to prevent saving layout for the closed window
        ctx.last_window = INVALID_WINDOW;
    }
}

/** Keyboard layout change handler. */
static void on_layout_change(unsigned long layout)
{
    ctx.current_layout = layout;
    clock_gettime(CLOCK_MONOTONIC, &ctx.switch_timestamp);
}

/**
 * Application entry point.
 */
int main(int argc, char* argv[])
{
    const struct option long_opts[] = {
        { "default", required_argument, NULL, 'd' },
        { "timeout", required_argument, NULL, 't' },
        { "version", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      0,                 NULL,  0  }
    };
    const char* short_opts = "d:t:vh";

    opterr = 0; // prevent native error messages

    // parse arguments
    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd':
                ctx.default_layout = atoi(optarg);
                if (ctx.default_layout < -1 || ctx.default_layout > 0xffff) {
                    fprintf(stderr, "Invalid default layout: %s\n",
                            argv[optind - 1]);
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                ctx.switch_timeout = atoi(optarg);
                if (ctx.switch_timeout < 0) {
                    fprintf(stderr, "Invalid timeout value: %s\n",
                            argv[optind - 1]);
                    return EXIT_FAILURE;
                }
                break;
            case 'v':
                printf("swaykbdd version " VERSION ".\n");
                return EXIT_SUCCESS;
            case 'h':
                printf("Keyboard layout switcher for Sway.\n");
                printf("Usage: %s [OPTION]\n", argv[0]);
                printf("  -d, --default ID  Default layout for new windows [%i]\n",
                       DEFAULT_LAYOUT);
                printf("  -t, --timeout MS  Delay between switching and saving layout [%i ms]\n",
                       DEFAULT_TIMEOUT);
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

    return sway_monitor(on_focus_change, on_window_close, on_layout_change);
}
