// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"

#include <stdlib.h>

/** Layouts map. */
struct layout {
    int window;
    int layout;
};

static struct layout* layouts;
static int layouts_sz;

/**
 * Find layout info.
 * @param[in] window window Id
 * @return pointer to layout instance or NULL if not found
 */
struct layout* find_layout(int window)
{
    for (int i = 0; i < layouts_sz; ++i) {
        if (layouts[i].window == window) {
            return &layouts[i];
        }
    }
    return NULL;
}

int get_layout(int window)
{
    const struct layout* l = find_layout(window);
    return l ? l->layout : -1;
}

void put_layout(int window, int layout)
{
    // search for existing description
    struct layout* existing = find_layout(window);
    if (existing) {
        if (layout == -1) {
            existing->window = -1;
        } else {
            existing->layout = layout;
        }
        return;
    }
    if (layout == -1) {
        return; // remove not existed window
    }

    // search for free description
    for (int i = 0; i < layouts_sz; ++i) {
        struct layout* l = &layouts[i];
        if (l->window == -1) {
            l->window = window;
            l->layout = layout;
            return;
        }
    }

    // realloc
    const int old_sz = layouts_sz;
    layouts_sz += 8;
    layouts = realloc(layouts, layouts_sz * sizeof(struct layout));
    for (int i = old_sz; i < layouts_sz; ++i) {
        struct layout* l = &layouts[i];
        if (i == old_sz) {
            l->window = window;
            l->layout = layout;
        } else {
            l->window = -1;
        }
    }
}
