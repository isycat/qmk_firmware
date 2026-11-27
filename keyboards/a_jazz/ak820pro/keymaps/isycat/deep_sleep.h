/* Copyright 2026 isycat
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

// Put the MCU into Deep-sleep until a key, the knob or a switch moves.
// Blocks until wake-up. Call only with the lights and screen already blanked.
void deep_sleep_enter(void);
