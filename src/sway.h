// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

/**
 * Callback function: Window focus change handler.
 * @param[in] id identifier of currently focused window (container)
 * @return keyboard layout to set, -1 to leave current
 */
typedef int (*on_focus)(int id);

/**
 * Callback function: Window close handler.
 * @param[in] id identifier of closed window (container)
 */
typedef void (*on_close)(int id);

/**
 * Callback function: Keyboard layout change handler.
 * @param[in] index current keyboard layout index
 */
typedef void (*on_layout)(int index);

/**
 * Connect to Sway IPC and start event monitoring.
 * Function never returns unless errors occurred.
 * @param[in] fn_focus event handler for focus change
 * @param[in] fn_close event handler for window close
 * @param[in] fn_layout event handler for layout change
 * @return error code
 */
int sway_monitor(on_focus fn_focus, on_close fn_close, on_layout fn_layout);
