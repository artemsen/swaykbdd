// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#define INVALID_KEY     0
#define INVALID_LAYOUT -1

/**
 * Get layout information for specified window.
 * @param[in] key window key
 * @return layout index, INVALID_LAYOUT if not found
 */
int get_layout(unsigned long key);

/**
 * Put layout information into storage.
 * @param[in] key window key
 * @param[in] layout keyboard layout index
 */
void put_layout(unsigned long key, int layout);

/**
 * Remove layout information from storage.
 * @param[in] key window key
 */
void rm_layout(unsigned long key);
