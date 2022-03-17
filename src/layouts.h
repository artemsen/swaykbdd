// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

/**
 * Get layout information for specified window.
 * @param[in] window window Id
 * @return layout index, -1 if not found
 */
int get_layout(int window);

/**
 * Put layout information into storage.
 * @param[in] window window Id
 * @param[in] layout keyboard layout index
 */
void put_layout(int window, int layout);

/**
 * Remove layout information from storage.
 * @param[in] window window Id
 */
void rm_layout(int window);
