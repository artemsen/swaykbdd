// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"
#include "sway.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/** Current keyboard layout (system wide). */
static int current_layout;

/** Last focused window. */
static int last_window = -1;

/** Focus change handler. */
static int on_focus_change(int id)
{
    if (last_window == -1) {
        // first focus
        last_window = id;
        return -1;
    }

    // save current layout for previous window
    put_layout(last_window, current_layout);

    // get layout for current window
    int layout = get_layout(id);

    if (layout < 0) {
        layout = 0; // set default
    }
    if (layout == current_layout) {
        layout = -1; // already set
    }

    last_window = id;

    return layout;
}

/** Window close handler. */
static void on_window_close(int id)
{
    put_layout(id, -1);
}

/** Keyboard layout change handler. */
static void on_layout_change(int index)
{
    current_layout = index;
}

/**
 * Application entry point.
 */
int main(int argc, char* argv[])
{
    const struct option long_opts[] = {
        { "version", no_argument, NULL, 'v' },
        { "help",    no_argument, NULL, 'h' },
        { NULL,      0,           NULL,  0  }
    };
    const char* short_opts = "vh";

    opterr = 0; // prevent native error messages

    // parse arguments
    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'v':
                printf("%s version " VERSION ".", argv[0]);
                return EXIT_SUCCESS;
            case 'h':
                printf("Usage: %s [OPTION]", argv[0]);
                puts("  -v, --version   Print version info and exit");
                puts("  -h, --help      Print this help and exit");
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
