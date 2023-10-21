// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#include <stdint.h>

#define INVALID_LAYOUT -1

/**
 * Get layout information for specified window.
 * @param[in] window window id
 * @param[in] tab subwindow (tab) id
 * @return layout index, INVALID_LAYOUT if not found
 */
int get_layout(uint32_t window, uint32_t tab);

/**
 * Put layout information into storage.
 * @param[in] window window id
 * @param[in] tab subwindow (tab) id
 * @param[in] layout keyboard layout index
 */
void put_layout(uint32_t window, uint32_t tab, int layout);

/**
 * Remove layout information from storage.
 * @param[in] window window id
 */
void rm_layout(uint32_t window);
