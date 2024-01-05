// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

/**
 * Callback function: Window focus change handler.
 * @param[in] wnd_id identifier of currently focused window (container)
 * @param[in] app_id application id
 * @param[in] title title of the window
 * @return keyboard layout to set, -1 to leave the current one
 */
typedef int (*on_focus)(int wnd_id, const char* app_id, const char* title);

/**
 * Callback function: Window title change handler.
 * @param[in] wnd_id identifier of currently focused window (container)
 * @param[in] app_id application id
 * @param[in] title title of the window
 * @return keyboard layout to set, -1 to leave the current one
 */
typedef int (*on_title)(int wnd_id, const char* app_id, const char* title);

/**
 * Callback function: Window close handler.
 * @param[in] wnd_id identifier of currently focused window (container)
 * @return keyboard layout to set, -1 to leave the current one
 */
typedef int (*on_close)(int wnd_id);

/**
 * Callback function: Keyboard layout change handler.
 * @param[in] layout current keyboard layout index
 */
typedef void (*on_layout)(int layout);

/**
 * Connect to Sway IPC and start event monitoring.
 * Function never returns unless errors occurred.
 * @param[in] fn_focus event handler for focus change
 * @param[in] fn_title event handler for title change
 * @param[in] fn_close event handler for window close
 * @param[in] fn_layout event handler for layout change
 * @return error code
 */
int sway_monitor(on_focus fn_focus, on_title fn_title,
                 on_close fn_close, on_layout fn_layout);
