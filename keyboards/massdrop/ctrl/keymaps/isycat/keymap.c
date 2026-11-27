/* Drop CTRL v1 (massdrop/ctrl) VIA keymap for isycat.
 *
 * Replaces the AutoHotkey macros with real firmware behaviour:
 *   - Navigation remaps (Ctrl+PgUp/PgDn, Ctrl/Alt+[Shift]+Left/Right, Ctrl+Alt+Left/Right,
 *     Win+Ctrl+Space emoji picker)
 *   - Pause key tap dance: 1 tap = Play/Pause, 2 taps = Next track
 *   - AltGr Swedish letters: RAlt + a/[ = å, RAlt + o/; = ö, RAlt + e/' = ä (Shift = capital)
 *   - Scroll Lock state shown on the Scroll Lock key LED (red by default)
 *   - Lighting zones (all / keys / underglow / off) that keep the indicator working
 *   - All of the above configurable from VIA via a custom definition JSON (via/ folder)
 *
 * Based on the VIA keymap from the-via/qmk_userspace_via (InonL, dsluo). GPL-2.0-or-later.
 */
#include QMK_KEYBOARD_H

enum ctrl_keycodes {
    U_T_AUTO = QK_KB_0, // USB Extra Port Toggle Auto Detect / Always Active
    U_T_AGCR,           // USB Toggle Automatic GCR control
    DBG_TOG,            // DEBUG Toggle On / Off
    DBG_MTRX,           // DEBUG Toggle Matrix Prints
    DBG_KBD,            // DEBUG Toggle Keyboard Prints
    DBG_MOU,            // DEBUG Toggle Mouse Prints
    MD_BOOT,            // Restart into bootloader after hold timeout
    CK_PPTD,            // Play/Pause on tap, Next track on double tap
    CK_SWALT,           // Right Alt with Swedish letters (å ä ö)
    CK_LZONE,           // Cycle lighting zones (all / keys / underglow / off)
};

#include "layers.h"

/* ------------------------------------------------------------------------- */
/* Persistent settings (EEPROM user datablock)                               */
/* ------------------------------------------------------------------------- */

enum lighting_zone { ZONE_ALL = 0, ZONE_KEYS, ZONE_UNDERGLOW, ZONE_OFF, ZONE_COUNT };

typedef struct {
    uint8_t magic;
    uint8_t zone;         // enum lighting_zone
    uint8_t ind_enabled;  // Scroll Lock indicator on/off
    uint8_t ind_hue;      // Scroll Lock indicator colour
    uint8_t ind_sat;
    uint8_t ind_val;      // Scroll Lock indicator brightness
    uint8_t nav_enabled;  // Navigation remaps on/off
    uint8_t swe_enabled;  // Swedish AltGr letters on/off
    uint8_t td_term_10ms; // Pause double-tap window in 10 ms units
    uint8_t reserved[3];
} user_config_t;

_Static_assert(sizeof(user_config_t) == EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE mismatch");

#define USER_CONFIG_MAGIC 0xC7

static user_config_t cfg;

static void cfg_defaults(void) {
    cfg = (user_config_t){
        .magic        = USER_CONFIG_MAGIC,
        .zone         = ZONE_ALL,
        .ind_enabled  = 1,
        .ind_hue      = 0,
        .ind_sat      = 255,
        .ind_val      = 255,
        .nav_enabled  = 1,
        .swe_enabled  = 1,
        .td_term_10ms = 30,
    };
}

static void cfg_save(void) {
    eeconfig_update_user_datablock(&cfg);
}

void eeconfig_init_user(void) {
    cfg_defaults();
    cfg_save();
}

/* ------------------------------------------------------------------------- */
/* Lighting zones + Scroll Lock indicator                                    */
/* ------------------------------------------------------------------------- */

#define SCROLL_LOCK_LED 14 // g_led_config matrix [6][6]

// The VIA definition (via/drop_ctrl_v1_isycat.json) lists effects by index.
_Static_assert(RGB_MATRIX_EFFECT_MAX == 45, "RGB effect list changed: update the VIA definition JSON");

static void zone_apply(void) {
    switch (cfg.zone) {
        case ZONE_KEYS:
            rgb_matrix_set_flags_noeeprom(LED_FLAG_KEYLIGHT | LED_FLAG_MODIFIER | LED_FLAG_INDICATOR);
            break;
        case ZONE_UNDERGLOW:
            rgb_matrix_set_flags_noeeprom(LED_FLAG_UNDERGLOW);
            break;
        default:
            rgb_matrix_set_flags_noeeprom(LED_FLAG_ALL);
            break;
    }
    rgb_matrix_set_color_all(0, 0, 0);
    // The matrix itself always stays enabled so the indicator can still render;
    // ZONE_OFF is implemented by blanking everything in the indicator pass.
    if (!rgb_matrix_is_enabled()) rgb_matrix_enable_noeeprom();
}

static void zone_set(uint8_t zone, bool save) {
    cfg.zone = zone % ZONE_COUNT;
    zone_apply();
    if (save) cfg_save();
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (cfg.zone == ZONE_OFF) {
        for (uint8_t i = led_min; i < led_max; i++) rgb_matrix_set_color(i, 0, 0, 0);
    }
    if (cfg.ind_enabled && host_keyboard_led_state().scroll_lock) {
        RGB rgb = hsv_to_rgb((HSV){cfg.ind_hue, cfg.ind_sat, cfg.ind_val});
        RGB_MATRIX_INDICATOR_SET_COLOR(SCROLL_LOCK_LED, rgb.r, rgb.g, rgb.b);
    }
    // Skip Massdrop's colour-inverting lock indicators.
    return false;
}

/* ------------------------------------------------------------------------- */
/* Pause tap dance                                                           */
/* ------------------------------------------------------------------------- */

static uint8_t  pp_taps;
static uint16_t pp_timer;

static void pp_fire(void) {
    if (pp_taps == 1) tap_code16(KC_MPLY);
    if (pp_taps >= 2) tap_code16(KC_MNXT);
    pp_taps = 0;
}

static void pp_press(void) {
    pp_taps++;
    pp_timer = timer_read();
    if (pp_taps >= 2) pp_fire(); // no triple-tap action, so fire right away
}

static void pp_task(void) {
    if (pp_taps && timer_elapsed(pp_timer) >= (uint16_t)cfg.td_term_10ms * 10) pp_fire();
}

/* ------------------------------------------------------------------------- */
/* Swedish letters on Right Alt                                              */
/* ------------------------------------------------------------------------- */
/* Right Alt is held back from the host until another key is pressed. If that
 * key is one of the Swedish letter keys, the letter is typed with a Windows
 * Alt code (Alt + numpad 0229 etc.) and Right Alt never reaches the host.
 * Otherwise Right Alt is sent first and the key works as a normal Alt combo.
 * A lone tap of Right Alt sends nothing (same as the old AHK script). */

static bool    swalt_held;
static bool    swalt_sent;
static uint8_t swe_swallowed; // bit per letter key whose release must be swallowed

static void send_alt_code(const char *digits) {
    uint8_t saved = get_mods();
    uint8_t weak  = get_weak_mods();
    clear_mods();
    clear_weak_mods();
    send_keyboard_report();

    bool numlock_off = !host_keyboard_led_state().num_lock;
    if (numlock_off) tap_code(KC_NUM);

    register_code(KC_LALT);
    for (const char *p = digits; *p; p++) {
        tap_code(*p == '0' ? KC_KP_0 : (KC_KP_1 + (*p - '1')));
    }
    unregister_code(KC_LALT);

    if (numlock_off) tap_code(KC_NUM);

    set_mods(saved);
    set_weak_mods(weak);
    send_keyboard_report();
}

// Returns the bit index for a Swedish letter key, or -1.
static int8_t swe_index(uint16_t keycode) {
    switch (keycode) {
        case KC_A: return 0;
        case KC_LBRC: return 1; // å
        case KC_O: return 2;
        case KC_SCLN: return 3; // ö
        case KC_E: return 4;
        case KC_QUOT: return 5; // ä
        default: return -1;
    }
}

static void swe_type(int8_t idx, bool upper) {
    // Windows-1252 Alt codes
    static const char *const lower_codes[] = {"0229", "0246", "0228"}; // å ö ä
    static const char *const upper_codes[] = {"0197", "0214", "0196"}; // Å Ö Ä
    uint8_t letter = idx / 2;
    send_alt_code(upper ? upper_codes[letter] : lower_codes[letter]);
}

static void swalt_send(void) {
    if (swalt_held && !swalt_sent) {
        register_code(KC_RALT);
        swalt_sent = true;
    }
}

/* ------------------------------------------------------------------------- */
/* Navigation remaps                                                         */
/* ------------------------------------------------------------------------- */
/* Each remap swaps the output key and adjusts modifiers while the key is held.
 * Modifiers taken away are kept away until the user releases them or presses an
 * unrelated key, so that the host never sees a lone Alt tap (which would open
 * the menu bar in many Windows apps). */

static uint8_t  nav_suppressed; // real modifiers currently hidden from the host
static uint8_t  nav_added;      // modifiers added for the active remap
static uint16_t nav_trigger;    // physical keycode that started the active remap
static uint16_t nav_output;     // keycode registered for the active remap

static void nav_restore(void) {
    if (nav_suppressed) {
        add_mods(nav_suppressed);
        nav_suppressed = 0;
        send_keyboard_report();
    }
}

static bool nav_lookup(uint16_t keycode, uint8_t mods, uint16_t *out, uint8_t *remove, uint8_t *add) {
    bool ctrl  = mods & MOD_MASK_CTRL;
    bool alt   = mods & MOD_MASK_ALT;
    bool shift = mods & MOD_MASK_SHIFT;
    bool gui   = mods & MOD_MASK_GUI;
    bool left  = keycode == KC_LEFT;

    *remove = 0;
    *add    = 0;
    switch (keycode) {
        case KC_PGUP:
        case KC_PGDN:
            // Ctrl+PgUp/PgDn -> Ctrl+Home/End (other modifiers pass through)
            if (!ctrl) return false;
            *out = keycode == KC_PGUP ? KC_HOME : KC_END;
            return true;
        case KC_LEFT:
        case KC_RGHT:
            if (gui) return false;
            if (ctrl && !alt) {
                // Ctrl+[Shift]+Left/Right -> [Shift]+Home/End
                *out    = left ? KC_HOME : KC_END;
                *remove = mods & MOD_MASK_CTRL;
                return true;
            }
            if (alt && !ctrl) {
                // Alt+[Shift]+Left/Right -> Ctrl+[Shift]+Left/Right (word jump)
                *out    = keycode;
                *remove = mods & MOD_MASK_ALT;
                *add    = MOD_BIT(KC_LCTL);
                return true;
            }
            if (alt && ctrl && !shift) {
                // Ctrl+Alt+Left/Right -> Ctrl+Shift+Tab / Ctrl+Tab
                *out    = KC_TAB;
                *remove = mods & MOD_MASK_ALT;
                *add    = left ? MOD_BIT(KC_LSFT) : 0;
                return true;
            }
            return false;
        case KC_SPC:
            // Win+Ctrl+Space -> Win+; (Windows emoji picker)
            if (!(ctrl && gui) || alt || shift) return false;
            *out    = KC_SCLN;
            *remove = mods & MOD_MASK_CTRL;
            return true;
        default:
            return false;
    }
}

static bool nav_process(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        uint16_t out;
        uint8_t  remove, add;
        uint8_t  mods = get_mods() | nav_suppressed;
        if (nav_trigger || !cfg.nav_enabled || !nav_lookup(keycode, mods, &out, &remove, &add)) {
            nav_restore();
            return true;
        }
        // Mods hidden by an earlier remap but not needed hidden now come back.
        uint8_t restore = nav_suppressed & ~remove;
        if (restore) add_mods(restore);
        nav_suppressed &= remove;

        // Add new modifiers before removing Alt so Windows never sees a lone Alt.
        add &= ~get_mods();
        if (add) {
            add_mods(add);
            send_keyboard_report();
        }
        uint8_t now_remove = remove & get_mods();
        if (now_remove) {
            del_mods(now_remove);
            nav_suppressed |= now_remove;
        }
        send_keyboard_report();

        register_code(out);
        nav_trigger = keycode;
        nav_output  = out;
        nav_added   = add;
        return false;
    }

    if (keycode == nav_trigger) {
        unregister_code(nav_output);
        if (nav_added) {
            del_mods(nav_added);
            send_keyboard_report();
        }
        nav_trigger = 0;
        nav_output  = 0;
        nav_added   = 0;
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/* Key processing                                                            */
/* ------------------------------------------------------------------------- */

static bool is_basic_mod(uint16_t keycode) {
    return keycode >= KC_LEFT_CTRL && keycode <= KC_RIGHT_GUI;
}

#define MODS_SHIFT (get_mods() & MOD_MASK_SHIFT)
#define MODS_CTRL (get_mods() & MOD_MASK_CTRL)

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static uint32_t boot_timer;
    bool            pressed = record->event.pressed;

    // Track modifier releases so hidden modifiers are not restored after release.
    if (!pressed && is_basic_mod(keycode)) {
        nav_suppressed &= ~MOD_BIT(keycode);
    }

    // --- Right Alt / Swedish letters -------------------------------------
    // A plain Right Alt in the keymap gets the same treatment, so the feature
    // survives loading an older VIA layout that still has KC_RALT there.
    if (keycode == CK_SWALT || (keycode == KC_RALT && cfg.swe_enabled)) {
        if (pressed) {
            swalt_held = true;
            swalt_sent = false;
        } else {
            if (swalt_sent) unregister_code(KC_RALT);
            nav_suppressed &= ~MOD_BIT(KC_RALT);
            swalt_held = false;
            swalt_sent = false;
        }
        return false;
    }
    {
        int8_t idx = swe_index(keycode);
        if (idx >= 0 && !pressed && (swe_swallowed & (1 << idx))) {
            swe_swallowed &= ~(1 << idx);
            return false;
        }
        if (pressed && swalt_held && !swalt_sent) {
            if (idx >= 0 && cfg.swe_enabled) {
                swe_type(idx, MODS_SHIFT);
                swe_swallowed |= (1 << idx);
                return false;
            }
            if (!is_basic_mod(keycode)) swalt_send();
        }
    }

    // --- Navigation remaps -----------------------------------------------
    if (!is_basic_mod(keycode)) {
        bool is_trigger = keycode == KC_PGUP || keycode == KC_PGDN || keycode == KC_LEFT || keycode == KC_RGHT || keycode == KC_SPC;
        if (pressed && !is_trigger) nav_restore();
        if (is_trigger && !nav_process(keycode, record)) return false;
    }

    switch (keycode) {
        case CK_PPTD:
            if (pressed) pp_press();
            return false;
        case CK_LZONE:
        case QK_UNDERGLOW_TOGGLE:
        case QK_RGB_MATRIX_TOGGLE:
            if (pressed) zone_set(cfg.zone + 1, true);
            return false;
        case QK_RGB_MATRIX_OFF:
            if (pressed) zone_set(ZONE_OFF, true);
            return false;
        case QK_RGB_MATRIX_ON:
            if (pressed) zone_set(ZONE_ALL, true);
            return false;
        case U_T_AUTO:
            if (pressed && MODS_SHIFT && MODS_CTRL) {
                TOGGLE_FLAG_AND_PRINT(usb_extra_manual, "USB extra port manual mode");
            }
            return false;
        case U_T_AGCR:
            if (pressed && MODS_SHIFT && MODS_CTRL) {
                TOGGLE_FLAG_AND_PRINT(usb_gcr_auto, "USB GCR auto mode");
            }
            return false;
        case DBG_TOG:
            if (pressed) TOGGLE_FLAG_AND_PRINT(debug_enable, "Debug mode");
            return false;
        case DBG_MTRX:
            if (pressed) TOGGLE_FLAG_AND_PRINT(debug_matrix, "Debug matrix");
            return false;
        case DBG_KBD:
            if (pressed) TOGGLE_FLAG_AND_PRINT(debug_keyboard, "Debug keyboard");
            return false;
        case DBG_MOU:
            if (pressed) TOGGLE_FLAG_AND_PRINT(debug_mouse, "Debug mouse");
            return false;
        case MD_BOOT:
            if (pressed) {
                boot_timer = timer_read32();
            } else if (timer_elapsed32(boot_timer) >= 500) {
                reset_keyboard();
            }
            return false;
        default:
            return true;
    }
}

void housekeeping_task_user(void) {
    pp_task();
}

void keyboard_post_init_user(void) {
    eeconfig_read_user_datablock(&cfg);
    if (!eeconfig_is_user_datablock_valid() || cfg.magic != USER_CONFIG_MAGIC) {
        eeconfig_init_user();
    }
    zone_apply();
}

/* ------------------------------------------------------------------------- */
/* VIA custom menu (channel 0)                                               */
/* ------------------------------------------------------------------------- */
/* Value ids must match via/drop_ctrl_v1_isycat.json */
enum via_isycat_value {
    id_zone = 1,
    id_ind_enabled,
    id_ind_color,
    id_ind_brightness,
    id_nav_enabled,
    id_swe_enabled,
    id_td_term,
};

static void isycat_get(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_zone: v[0] = cfg.zone; break;
        case id_ind_enabled: v[0] = cfg.ind_enabled; break;
        case id_ind_color: v[0] = cfg.ind_hue; v[1] = cfg.ind_sat; break;
        case id_ind_brightness: v[0] = cfg.ind_val; break;
        case id_nav_enabled: v[0] = cfg.nav_enabled; break;
        case id_swe_enabled: v[0] = cfg.swe_enabled; break;
        case id_td_term: v[0] = cfg.td_term_10ms; break;
    }
}

static void isycat_set(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_zone: zone_set(v[0], false); break;
        case id_ind_enabled: cfg.ind_enabled = v[0] ? 1 : 0; break;
        case id_ind_color: cfg.ind_hue = v[0]; cfg.ind_sat = v[1]; break;
        case id_ind_brightness: cfg.ind_val = v[0]; break;
        case id_nav_enabled: cfg.nav_enabled = v[0] ? 1 : 0; break;
        case id_swe_enabled: cfg.swe_enabled = v[0] ? 1 : 0; break;
        case id_td_term: cfg.td_term_10ms = v[0] < 10 ? 10 : v[0]; break;
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data... ]
    uint8_t *command_id = &data[0];
    uint8_t *channel_id = &data[1];
    uint8_t *value_id   = &data[2];
    uint8_t *value_data = &data[3];

    if (*channel_id != id_custom_channel) {
        *command_id = id_unhandled;
        return;
    }
    switch (*command_id) {
        case id_custom_set_value: isycat_set(*value_id, value_data); break;
        case id_custom_get_value: isycat_get(*value_id, value_data); break;
        case id_custom_save: cfg_save(); break;
        default: *command_id = id_unhandled; break;
    }
}

/* VIA's built-in RGB matrix channel turns the whole matrix off for effect 0,
 * which would also kill the Scroll Lock indicator. Map that to ZONE_OFF and
 * keep the previous effect running underneath instead. */
void via_custom_value_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &data[0];
    uint8_t *channel_id = &data[1];
    uint8_t *value_id   = &data[2];
    uint8_t *value_data = &data[3];

    if (*channel_id == id_qmk_rgb_matrix_channel && *value_id == id_qmk_rgb_matrix_effect) {
        if (*command_id == id_custom_set_value) {
            if (value_data[0] == 0) {
                zone_set(ZONE_OFF, false);
            } else {
                if (cfg.zone == ZONE_OFF) zone_set(ZONE_ALL, false);
                rgb_matrix_mode_noeeprom(value_data[0]);
            }
            return;
        }
        if (*command_id == id_custom_get_value) {
            value_data[0] = cfg.zone == ZONE_OFF ? 0 : rgb_matrix_get_mode();
            return;
        }
        if (*command_id == id_custom_save) {
            eeconfig_update_rgb_matrix();
            cfg_save();
            return;
        }
    }
    if (*channel_id == id_qmk_rgb_matrix_channel) {
        via_qmk_rgb_matrix_command(data, length);
        if (*command_id == id_custom_save) cfg_save();
        return;
    }
    via_custom_value_command_kb(data, length);
}
