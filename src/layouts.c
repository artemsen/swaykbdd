// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"

#include <stdlib.h>

/** State descriptor: window and its layout. */
struct state {
    int window;
    int layout;
};
static struct state* states;
static int states_sz;

/**
 * Find state description for specified window.
 * @param[in] window window Id
 * @return pointer to the state descriptor or NULL if not found
 */
static struct state* find_state(int window)
{
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->window == window) {
            return entry;
        }
    }
    return NULL;
}

int get_layout(int window)
{
    const struct state* entry = find_state(window);
    return entry ? entry->layout : -1;
}

void put_layout(int window, int layout)
{
    // search for existing descriptor
    struct state* entry = find_state(window);
    if (entry) {
        entry->layout = layout;
        return;
    }

    // search for free descriptor
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->window == -1) {
            entry->window = window;
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
            entry->layout = layout;
        } else {
            entry->window = -1;
        }
    }
}

void rm_layout(int window)
{
    struct state* entry = find_state(window);
    if (entry) {
        // mark descriptor as free cell
        entry->window = -1;
        entry->layout = -1;
    }
}
