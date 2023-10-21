// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"

#include <stdlib.h>

/** State descriptor: window/tab and its layout. */
struct state {
    uint32_t window;
    uint32_t tab;
    int      layout;
};
static struct state* states;
static int states_sz;

/**
 * Find state description for specified window key.
 * @param[in] window window id
 * @param[in] tab subwindow (tab) id
 * @return pointer to the state descriptor or NULL if not found
 */
static struct state* find_state(uint32_t window, uint32_t tab)
{
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->window == window && entry->tab == tab) {
            return entry;
        }
    }
    return NULL;
}

int get_layout(uint32_t window, uint32_t tab)
{
    const struct state* entry = find_state(window, tab);
    return entry ? entry->layout : INVALID_LAYOUT;
}

void put_layout(uint32_t window, uint32_t tab, int layout)
{
    // search for existing descriptor
    struct state* entry = find_state(window, tab);
    if (entry) {
        entry->layout = layout;
        return;
    }

    // search for free descriptor
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->window == 0 && entry->tab == 0) {
            entry->window = window;
            entry->tab = tab;
            entry->layout = layout;
            return;
        }
    }

    // realloc
    const int old_sz = states_sz;
    states_sz += 8;
    states = realloc(states, states_sz * sizeof(struct state));
    for (int i = old_sz; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (i == old_sz) {
            entry->window = window;
            entry->tab = tab;
            entry->layout = layout;
        } else {
            entry->window = 0;
            entry->tab = 0;
            entry->layout = INVALID_LAYOUT;
        }
    }
}

void rm_layout(uint32_t window)
{
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->window == window) {
            entry->window = 0;
            entry->tab = 0;
            entry->layout = INVALID_LAYOUT;
        }
    }
}
