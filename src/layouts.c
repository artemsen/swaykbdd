// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "layouts.h"

#include <stdlib.h>

/** State descriptor: window key and its layout. */
struct state {
    unsigned long key;
    int layout;
};
static struct state* states;
static int states_sz;

/**
 * Find state description for specified window key.
 * @param[in] window window Id
 * @return pointer to the state descriptor or NULL if not found
 */
static struct state* find_state(unsigned long key)
{
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->key == key) {
            return entry;
        }
    }
    return NULL;
}

int get_layout(unsigned long key)
{
    const struct state* entry = find_state(key);
    return entry ? entry->layout : INVALID_LAYOUT;
}

void put_layout(unsigned long key, int layout)
{
    // search for existing descriptor
    struct state* entry = find_state(key);
    if (entry) {
        entry->layout = layout;
        return;
    }

    // search for free descriptor
    for (int i = 0; i < states_sz; ++i) {
        struct state* entry = &states[i];
        if (entry->key == INVALID_KEY) {
            entry->key = key;
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
            entry->key = key;
            entry->layout = layout;
        } else {
            entry->key = INVALID_KEY;
        }
    }
}

void rm_layout(unsigned long key)
{
    struct state* entry = find_state(key);
    if (entry) {
        // mark descriptor as free cell
        entry->key = INVALID_KEY;
        entry->layout = INVALID_LAYOUT;
    }
}
