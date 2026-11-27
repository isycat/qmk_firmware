/* Copyright 2022 dsluo (me@dsluo.dev), 2026 isycat
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

// Enable additional LED functionality
#define USB_LED_INDICATOR_ENABLE
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_KEYPRESSES

// Persistent settings for the keymap (see user_config_t in keymap.c)
#define EECONFIG_USER_DATA_SIZE 121

// Give Windows time to see each key of an Alt code
#define TAP_CODE_DELAY 5
