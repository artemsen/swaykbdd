// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#define INVALID_WINDOW  0
#define INVALID_LAYOUT -1

/**
 * Get layout information for specified window.
 * @param[in] window window Id
 * @return layout index, INVALID_LAYOUT if not found
 */
int get_layout(unsigned long window);

/**
 * Put layout information into storage.
 * @param[in] window window Id
 * @param[in] layout keyboard layout index
 */
void put_layout(unsigned long window, int layout);

/**
 * Remove layout information from storage.
 * @param[in] window window Id
 */
void rm_layout(unsigned long window);
