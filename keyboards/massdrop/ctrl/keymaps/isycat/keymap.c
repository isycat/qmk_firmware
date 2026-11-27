/* Drop CTRL v1 (massdrop/ctrl) VIA keymap for isycat.
 *
 * Replaces the AutoHotkey macros with real firmware behaviour:
 *   - Navigation remaps (Ctrl+PgUp/PgDn, Ctrl/Alt+[Shift]+Left/Right, Ctrl+Alt+Left/Right)
 *   - Pause key tap dance: 1 tap = Play/Pause, 2 taps = Next track
 *   - AltGr Swedish letters: RAlt + a/[ = å, RAlt + o/; = ö, RAlt + e/' = ä (Shift = capital)
 *   - Scroll Lock state shown on the Scroll Lock key LED (red by default)
 *   - Lighting zones (all / keys / underglow / off) that keep the indicator working
 *   - All of the above configurable from VIA via a custom definition JSON (via/ folder)
 *
 * Based on the VIA keymap from the-via/qmk_userspace_via (InonL, dsluo). GPL-2.0-or-later.
 */
#include QMK_KEYBOARD_H
#include "lib/lib8tion/lib8tion.h"

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
    CK_TH1,             // Tap/Hold 1..4: tap and hold keycodes set in VIA (Tweaks)
    CK_TH2,
    CK_TH3,
    CK_TH4,
    CK_GAME,            // Toggle game mode (F1 stays F1, optional remap bypass)
};

#include "layers.h"

/* ------------------------------------------------------------------------- */
/* Persistent settings (EEPROM user datablock)                               */
/* ------------------------------------------------------------------------- */

#define KEY_LED_COUNT 87 // LEDs 0..86 sit under keys, 87..118 are underglow
#define PALETTE_SIZE 15
#define TH_COUNT 4

enum game_flag {
    GAME_F1     = 1 << 0, // F1 sends f1_target outside game mode
    GAME_BYPASS = 1 << 1, // game mode switches off nav remaps and Swedish letters
    GAME_LED    = 1 << 2, // show game mode on the F1 key LED
    GAME_NUMLOCK = 1 << 3, // game mode follows the host's Num Lock (set by AHK)
};

enum lighting_zone { ZONE_ALL = 0, ZONE_KEYS, ZONE_UNDERGLOW, ZONE_OFF, ZONE_COUNT };

typedef struct __attribute__((packed)) {
    uint8_t magic;
    uint8_t zone;         // enum lighting_zone
    uint8_t ind_enabled;  // Scroll Lock indicator on/off
    uint8_t ind_hue;      // Scroll Lock indicator colour
    uint8_t ind_sat;
    uint8_t ind_val;      // Scroll Lock indicator brightness
    uint8_t nav_flags;    // NAV_* bits: which navigation remaps are on
    uint8_t swe_flags;    // SWE_* bits: Swedish letter options
    uint8_t td_term_10ms; // Pause double-tap window in 10 ms units
    uint8_t ug_mode;      // enum ug_mode
    uint8_t ug_hue;       // underglow colour for the UG_SOLID/UG_BREATHE modes
    uint8_t ug_sat;
    uint8_t ug_val;       // underglow brightness for its own modes
    uint8_t paint_on;     // show painted per-key colours
    uint8_t th_term_10ms; // Tap/Hold keys: hold threshold in 10 ms units
    uint8_t game_flags;   // GAME_* bits
    uint8_t palette[PALETTE_SIZE][3];     // hue, sat, brightness of brush colours 1..15
    uint8_t keymap[(KEY_LED_COUNT + 1) / 2]; // 4 bits per key LED: 0 = effect, n = palette[n-1]
    uint16_t th_tap[TH_COUNT];  // Tap/Hold keys: keycode sent on tap
    uint16_t th_hold[TH_COUNT]; // Tap/Hold keys: keycode held on hold
    uint16_t f1_target;         // what F1 sends outside game mode
    uint8_t  game_hue;          // game mode F1 LED colour
    uint8_t  game_sat;
    uint8_t  game_val;          // game mode F1 LED brightness
    uint8_t  ext_ver;           // 1 once the fields above have defaults
    uint8_t  future[33];        // headroom so later settings don't move VIA's data
} user_config_t;

_Static_assert(sizeof(user_config_t) == EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE mismatch");
// EEPROM budget: settings + VIA header (4) + 4 layers of keymap must leave >= 96 bytes for VIA macros.
_Static_assert(EECONFIG_SIZE + 4 + DYNAMIC_KEYMAP_LAYER_COUNT * MATRIX_ROWS * MATRIX_COLS * 2 + 96 <= EEPROM_SIZE,
               "Not enough EEPROM left for VIA macros");

#define USER_CONFIG_MAGIC 0xCA

enum ug_mode { UG_FOLLOW = 0, UG_SOLID, UG_BREATHE, UG_RAINBOW, UG_OFF, UG_COUNT };

enum nav_flag {
    NAV_PGUPDN    = 1 << 0, // Ctrl+PgUp/PgDn -> Ctrl+Home/End
    NAV_CTRL_ARR  = 1 << 1, // Ctrl+[Shift]+Left/Right -> [Shift]+Home/End
    NAV_ALT_ARR   = 1 << 2, // Alt+[Shift]+Left/Right -> Ctrl+[Shift]+Left/Right
    NAV_TAB_ARR   = 1 << 3, // Ctrl+Alt+Left/Right -> Ctrl+[Shift]+Tab
    NAV_ALL       = NAV_PGUPDN | NAV_CTRL_ARR | NAV_ALT_ARR | NAV_TAB_ARR,
};

enum swe_flag {
    SWE_ON         = 1 << 0, // Swedish letters on the AltGr SE key
    SWE_PLAIN_RALT = 1 << 1, // ...and on a plain Right Alt key
    SWE_ALT_KEYS   = 1 << 2, // [ ; ' as well as a o e
    SWE_ALL        = SWE_ON | SWE_PLAIN_RALT | SWE_ALT_KEYS,
};

static user_config_t cfg;
static bool          game_mode; // see Game mode section

static void cfg_defaults(void) {
    cfg = (user_config_t){
        .magic        = USER_CONFIG_MAGIC,
        .zone         = ZONE_ALL,
        .ind_enabled  = 1,
        .ind_hue      = 0,
        .ind_sat      = 255,
        .ind_val      = 255,
        .nav_flags    = NAV_ALL,
        .swe_flags    = SWE_ALL,
        .td_term_10ms = 30,
        .ug_mode      = UG_FOLLOW,
        .ug_hue       = 170,
        .ug_sat       = 255,
        .ug_val       = 128,
        .paint_on     = 1,
        .th_term_10ms = 20,
        .palette = {
            {0, 255, 255}, {21, 255, 255}, {43, 255, 255}, {64, 255, 255}, {85, 255, 255},
            {106, 255, 255}, {128, 255, 255}, {149, 255, 255}, {170, 255, 255}, {191, 255, 255},
            {213, 255, 255}, {234, 255, 255}, {0, 0, 255}, {28, 96, 255},
            {0, 0, 0}, // colour 15: off
        },
        // Tap/Hold 1 replaces MT(MOD_LGUI, KC_F1): hold = Win, tap = Alt+F23
        // (PowerToys Command Palette shortcut; F23 is otherwise unused)
        .th_tap  = {LALT(KC_F23), KC_NO, KC_NO, KC_NO},
        .th_hold = {KC_LGUI, KC_NO, KC_NO, KC_NO},
        .game_flags = GAME_F1 | GAME_LED | GAME_NUMLOCK,
        .f1_target  = LALT(KC_F23),
        .game_hue   = 85,
        .game_sat   = 255,
        .game_val   = 255,
        .ext_ver    = 1,
    };
}

static void cfg_save(void) {
    eeconfig_update_user_datablock(&cfg);
}

// Moving the settings block moves VIA's data too. If our settings are not valid
// (first boot or a layout change), make VIA reload the built-in keymap as well
// rather than reading shifted bytes.
// v7/v8 used a 121-byte settings block. Grow it in place and slide VIA's data
// (magic, layout options, keymap, macros) up behind it, so neither the
// settings nor the VIA keymap are lost.
#define OLD_USER_DATA_SIZE 121

static void migrate_from_121(void) {
    if (eeprom_read_dword(EECONFIG_USER) != OLD_USER_DATA_SIZE) return;
    uint8_t *block = EECONFIG_USER_DATABLOCK;
    if (eeprom_read_byte(block) != USER_CONFIG_MAGIC) return;

    uint16_t old_via = (uint16_t)(uintptr_t)block + OLD_USER_DATA_SIZE;
    uint16_t new_via = (uint16_t)(uintptr_t)block + EECONFIG_USER_DATA_SIZE;
    for (uint16_t i = EEPROM_SIZE - new_via; i-- > 0;) {
        eeprom_update_byte((uint8_t *)(uintptr_t)(new_via + i), eeprom_read_byte((uint8_t *)(uintptr_t)(old_via + i)));
    }

    user_config_t fresh;
    cfg_defaults();
    fresh = cfg;
    eeprom_read_block(&cfg, block, OLD_USER_DATA_SIZE);
    memcpy((uint8_t *)&cfg + OLD_USER_DATA_SIZE, (uint8_t *)&fresh + OLD_USER_DATA_SIZE, sizeof(cfg) - OLD_USER_DATA_SIZE);
    cfg.game_flags = fresh.game_flags; // was an unused byte in the 121-byte layout
    cfg_save(); // also writes the new version (size) marker
}

void via_init_kb(void) {
    migrate_from_121();
    user_config_t probe;
    eeconfig_read_user_datablock(&probe);
    if (!eeconfig_is_user_datablock_valid() || probe.magic != USER_CONFIG_MAGIC) {
        via_eeprom_set_valid(false);
    }
}

void eeconfig_init_user(void) {
    cfg_defaults();
    cfg_save();
}

/* ------------------------------------------------------------------------- */
/* Lighting zones + Scroll Lock indicator                                    */
/* ------------------------------------------------------------------------- */

#define SCROLL_LOCK_LED 14 // g_led_config matrix [6][6]
#define GAME_LED_INDEX 1   // F1

// The VIA definition (via/drop_ctrl_v1_isycat.json) lists effects by index.
_Static_assert(RGB_MATRIX_EFFECT_MAX == 45, "RGB effect list changed: update the VIA definition JSON");

static void zone_apply(void) {
    // Visibility is handled per LED in the indicator pass, so effects always draw everywhere.
    rgb_matrix_set_flags_noeeprom(LED_FLAG_ALL);
    // The matrix itself always stays enabled so the indicator can still render;
    // ZONE_OFF is implemented by blanking everything in the indicator pass.
    if (!rgb_matrix_is_enabled()) rgb_matrix_enable_noeeprom();
}

static void zone_set(uint8_t zone, bool save) {
    cfg.zone = zone % ZONE_COUNT;
    zone_apply();
    if (save) cfg_save();
}

/* --- Per-key paint ---------------------------------------------------- */

static bool     paint_mode;  // pressing keys paints instead of typing (not saved)
static uint8_t  paint_brush; // 0 = back to effect, 1..15 = palette colour
static uint16_t paint_timer;

#define PAINT_TIMEOUT_MS 60000 // paint mode switches itself off after a minute idle

static uint8_t key_color(uint8_t led) {
    uint8_t b = cfg.keymap[led / 2];
    return (led & 1) ? (b >> 4) : (b & 0x0F);
}

static void key_color_set(uint8_t led, uint8_t c) {
    uint8_t *b = &cfg.keymap[led / 2];
    *b = (led & 1) ? ((*b & 0x0F) | (c << 4)) : ((*b & 0xF0) | (c & 0x0F));
}

static void paint_all(uint8_t c) {
    for (uint8_t i = 0; i < KEY_LED_COUNT; i++) key_color_set(i, c);
}

static void paint_task(void) {
    if (paint_mode && timer_elapsed(paint_timer) > PAINT_TIMEOUT_MS) paint_mode = false;
}

/* --- Indicator pass: zones, underglow, painted keys, Scroll Lock ------- */

static RGB underglow_color(uint8_t led) {
    uint8_t t = scale16by8(g_rgb_timer, qadd8(rgb_matrix_get_speed() / 4, 1)) & 0xFF;
    switch (cfg.ug_mode) {
        case UG_BREATHE:
            return hsv_to_rgb((HSV){cfg.ug_hue, cfg.ug_sat, scale8(cfg.ug_val, sin8(t))});
        case UG_RAINBOW:
            return hsv_to_rgb((HSV){(uint8_t)(t + (led - KEY_LED_COUNT) * 8), 255, cfg.ug_val});
        default:
            return hsv_to_rgb((HSV){cfg.ug_hue, cfg.ug_sat, cfg.ug_val});
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    bool keys_visible = cfg.zone == ZONE_ALL || cfg.zone == ZONE_KEYS;
    bool ug_visible   = (cfg.zone == ZONE_ALL || cfg.zone == ZONE_UNDERGLOW) && cfg.ug_mode != UG_OFF;

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i >= KEY_LED_COUNT) {
            if (!ug_visible) {
                rgb_matrix_set_color(i, 0, 0, 0);
            } else if (cfg.ug_mode != UG_FOLLOW) {
                RGB rgb = underglow_color(i);
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            continue;
        }
        // Painted keys are their own layer: they keep their own colour and
        // brightness even when the effect is off, dark or hidden by the zone.
        uint8_t c = cfg.paint_on || paint_mode ? key_color(i) : 0;
        if (c) {
            const uint8_t *pc  = cfg.palette[c - 1];
            RGB            rgb = hsv_to_rgb((HSV){pc[0], pc[1], pc[2]});
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        } else if (!keys_visible) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }
    if (game_mode && (cfg.game_flags & GAME_LED)) {
        RGB rgb = hsv_to_rgb((HSV){cfg.game_hue, cfg.game_sat, cfg.game_val});
        RGB_MATRIX_INDICATOR_SET_COLOR(GAME_LED_INDEX, rgb.r, rgb.g, rgb.b);
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

static uint16_t numlock_quiet_timer; // Num Lock sync ignores the host for a moment after an Alt code

static void send_alt_code(const char *digits) {
    numlock_quiet_timer = timer_read() | 1;
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
    numlock_quiet_timer = timer_read() | 1;

    set_mods(saved);
    set_weak_mods(weak);
    send_keyboard_report();
}

// Returns the bit index for a Swedish letter key, or -1.
static int8_t swe_index(uint16_t keycode) {
    switch (keycode) {
        case KC_A: return 0;
        case KC_LBRC: return (cfg.swe_flags & SWE_ALT_KEYS) ? 1 : -1; // å
        case KC_O: return 2;
        case KC_SCLN: return (cfg.swe_flags & SWE_ALT_KEYS) ? 3 : -1; // ö
        case KC_E: return 4;
        case KC_QUOT: return (cfg.swe_flags & SWE_ALT_KEYS) ? 5 : -1; // ä
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

// Key tapped while Alt is held, right before Alt is hidden from the host.
// F22 does nothing in Windows or common apps (F23 is the Copilot key with Win+Shift).
#ifndef NAV_MASK_KEY
#    define NAV_MASK_KEY KC_F22
#endif

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
            if (!ctrl || !(cfg.nav_flags & NAV_PGUPDN)) return false;
            *out = keycode == KC_PGUP ? KC_HOME : KC_END;
            return true;
        case KC_LEFT:
        case KC_RGHT:
            if (gui) return false;
            if (ctrl && !alt && (cfg.nav_flags & NAV_CTRL_ARR)) {
                // Ctrl+[Shift]+Left/Right -> [Shift]+Home/End
                *out    = left ? KC_HOME : KC_END;
                *remove = mods & MOD_MASK_CTRL;
                return true;
            }
            if (alt && !ctrl && (cfg.nav_flags & NAV_ALT_ARR)) {
                // Alt+[Shift]+Left/Right -> Ctrl+[Shift]+Left/Right (word jump)
                *out    = keycode;
                *remove = mods & MOD_MASK_ALT;
                *add    = MOD_BIT(KC_LCTL);
                return true;
            }
            if (alt && ctrl && !shift && (cfg.nav_flags & NAV_TAB_ARR)) {
                // Ctrl+Alt+Left/Right -> Ctrl+Shift+Tab / Ctrl+Tab
                *out    = KC_TAB;
                *remove = mods & MOD_MASK_ALT;
                *add    = left ? MOD_BIT(KC_LSFT) : 0;
                return true;
            }
            return false;
        default:
            return false;
    }
}

static bool nav_process(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        uint16_t out;
        uint8_t  remove, add;
        uint8_t  mods = get_mods() | nav_suppressed;
        if (nav_trigger || !nav_lookup(keycode, mods, &out, &remove, &add)) {
            nav_restore();
            return true;
        }
        // Mods hidden by an earlier remap but not needed hidden now come back.
        uint8_t restore = nav_suppressed & ~remove;
        if (restore) add_mods(restore);
        nav_suppressed &= remove;

        // Windows treats an Alt release as a menu/access-key tap (and Alt+Shift as
        // the input-language hotkey) unless a non-modifier key went down while Alt
        // was held. Tap an inert mask key first, like AutoHotkey's MenuMaskKey.
        if (remove & get_mods() & MOD_MASK_ALT) {
            tap_code(NAV_MASK_KEY);
        }

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

/* ------------------------------------------------------------------------- */
/* Tap/Hold keys (configurable in VIA)                                       */
/* ------------------------------------------------------------------------- */
/* Like QMK mod-tap, but tap and hold can be any keycode VIA can pick,
 * including modified ones such as Alt+F5. Hold wins when the key is held past
 * the threshold or another key is pressed while it is down. */

enum th_state { TH_IDLE = 0, TH_UNDECIDED, TH_HOLDING };

static uint8_t  th_active = 0xFF; // slot currently down, 0xFF = none
static uint8_t  th_state;
static uint16_t th_timer;

static void th_hold_press(uint16_t kc) {
    if (IS_QK_MOMENTARY(kc)) {
        layer_on(QK_MOMENTARY_GET_LAYER(kc));
    } else if (kc != KC_NO) {
        register_code16(kc);
    }
}

static void th_hold_release(uint16_t kc) {
    if (IS_QK_MOMENTARY(kc)) {
        layer_off(QK_MOMENTARY_GET_LAYER(kc));
    } else if (kc != KC_NO) {
        unregister_code16(kc);
    }
}

static void th_go_hold(void) {
    if (th_active < TH_COUNT && th_state == TH_UNDECIDED) {
        th_state = TH_HOLDING;
        th_hold_press(cfg.th_hold[th_active]);
    }
}

static void th_task(void) {
    if (th_state == TH_UNDECIDED && timer_elapsed(th_timer) >= (uint16_t)cfg.th_term_10ms * 10) th_go_hold();
}

// Returns false if the event was a Tap/Hold key and has been handled.
static bool th_process(uint16_t keycode, keyrecord_t *record) {
    bool is_th = keycode >= CK_TH1 && keycode <= CK_TH4;
    if (!is_th) {
        if (record->event.pressed && th_state == TH_UNDECIDED) th_go_hold();
        return true;
    }
    uint8_t slot = keycode - CK_TH1;
    if (record->event.pressed) {
        if (th_state == TH_UNDECIDED) th_go_hold(); // another Tap/Hold key interrupts
        th_active = slot;
        th_state  = TH_UNDECIDED;
        th_timer  = timer_read();
    } else if (slot == th_active) {
        if (th_state == TH_UNDECIDED) {
            uint16_t tap = cfg.th_tap[slot];
            if (tap != KC_NO) tap_code16(tap);
        } else if (th_state == TH_HOLDING) {
            th_hold_release(cfg.th_hold[slot]);
        }
        th_active = 0xFF;
        th_state  = TH_IDLE;
    }
    return false;
}

/* ------------------------------------------------------------------------- */
/* Game mode + F1 remap                                                      */
/* ------------------------------------------------------------------------- */
/* Game mode is not saved: it is switched by the Game key, VIA, or the
 * host's Num Lock state, which a timer in the AHK script sets. */

static uint16_t f1_sent; // keycode registered by the current F1 press

static bool remaps_active(void) {
    return !(game_mode && (cfg.game_flags & GAME_BYPASS));
}

static void game_mode_set(bool on) {
    game_mode = on;
    if (!remaps_active()) nav_restore();
}

// With Num Lock sync on, the host's Num Lock is the game mode switch, so AHK
// can set it with SetNumLockState and the Game key just toggles Num Lock.
static void game_mode_toggle(void) {
    if (cfg.game_flags & GAME_NUMLOCK) {
        tap_code(KC_NUM);
    } else {
        game_mode_set(!game_mode);
    }
}

static void numlock_sync_task(void) {
    if (!(cfg.game_flags & GAME_NUMLOCK)) return;
    if (numlock_quiet_timer) {
        if (timer_elapsed(numlock_quiet_timer) < 400) return;
        numlock_quiet_timer = 0;
    }
    bool num = host_keyboard_led_state().num_lock;
    if (num != game_mode) game_mode_set(num);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static uint32_t boot_timer;
    bool            pressed = record->event.pressed;

    // Paint mode: keys paint their LED with the brush instead of typing.
    if (paint_mode) {
        if (pressed && record->event.key.row < MATRIX_ROWS && record->event.key.col < MATRIX_COLS) {
            uint8_t led = g_led_config.matrix_co[record->event.key.row][record->event.key.col];
            if (led < KEY_LED_COUNT) {
                key_color_set(led, paint_brush);
                cfg_save();
            }
            paint_timer = timer_read();
        }
        return false;
    }

    // Track modifier releases so hidden modifiers are not restored after release.
    if (!pressed && is_basic_mod(keycode)) {
        nav_suppressed &= ~MOD_BIT(keycode);
    }

    if (!th_process(keycode, record)) return false;

    if (keycode == CK_GAME) {
        if (pressed) game_mode_toggle();
        return false;
    }

    // F1 -> Alt+F23 (or whatever is set in VIA) outside game mode
    if (keycode == KC_F1) {
        if (pressed && (cfg.game_flags & GAME_F1) && !game_mode && cfg.f1_target != KC_NO) {
            f1_sent = cfg.f1_target;
            register_code16(f1_sent);
            return false;
        }
        if (!pressed && f1_sent) {
            unregister_code16(f1_sent);
            f1_sent = 0;
            return false;
        }
    }

    // Game mode bypass: AltGr SE is a plain Right Alt and no remaps apply.
    if (!remaps_active()) {
        if (keycode == CK_SWALT) {
            if (pressed) {
                register_code(KC_RALT);
            } else {
                unregister_code(KC_RALT);
            }
            return false;
        }
        if (!(swalt_held || swe_swallowed || nav_trigger)) goto skip_remaps;
    }

    // --- Right Alt / Swedish letters -------------------------------------
    // A plain Right Alt in the keymap gets the same treatment, so the feature
    // survives loading an older VIA layout that still has KC_RALT there.
    bool swe_on_ralt = (cfg.swe_flags & SWE_ON) && (cfg.swe_flags & SWE_PLAIN_RALT);
    if (keycode == CK_SWALT || (keycode == KC_RALT && (swe_on_ralt || (!pressed && swalt_held)))) {
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
            if (idx >= 0 && (cfg.swe_flags & SWE_ON)) {
                swe_type(idx, MODS_SHIFT);
                swe_swallowed |= (1 << idx);
                return false;
            }
            if (!is_basic_mod(keycode)) swalt_send();
        }
    }

    // --- Navigation remaps -----------------------------------------------
    if (!is_basic_mod(keycode)) {
        bool is_trigger = keycode == KC_PGUP || keycode == KC_PGDN || keycode == KC_LEFT || keycode == KC_RGHT;
        if (pressed && !is_trigger) nav_restore();
        if (is_trigger && !nav_process(keycode, record)) return false;
    }

skip_remaps:
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
    th_task();
    paint_task();
    numlock_sync_task();
}

void keyboard_post_init_user(void) {
    eeconfig_read_user_datablock(&cfg);
    if (!eeconfig_is_user_datablock_valid() || cfg.magic != USER_CONFIG_MAGIC) {
        eeconfig_init_user();
    }
    if (cfg.ext_ver != 1) { // v9 saved these bytes as zero
        cfg.game_hue = 85;
        cfg.game_sat = 255;
        cfg.game_val = 255;
        cfg.ext_ver  = 1;
        cfg_save();
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
    id_nav_pgupdn,
    id_swe_enabled,
    id_td_term,
    id_nav_ctrl_arrows,
    id_nav_alt_arrows,
    id_nav_tab_arrows,
    id_swe_plain_ralt,
    id_swe_alt_keys,
    id_ug_mode,
    id_ug_color,
    id_ug_brightness,
    id_paint_mode,
    id_paint_brush,
    id_paint_brush_color,
    id_paint_on,
    id_paint_tools,
};

#define id_palette_first 21      // ids 21..35 = palette colours 1..15 (hue, sat)
#define id_palette_val_first 36  // ids 36..50 = palette brightness 1..15
#define id_th_tap_first 51       // ids 51..54 = Tap/Hold tap keycodes
#define id_th_hold_first 55      // ids 55..58 = Tap/Hold hold keycodes
#define id_th_term 59
#define id_game_mode 60     // not saved; also used by the host helper
#define id_game_f1 61
#define id_game_f1_target 62
#define id_game_bypass 63
#define id_game_led 64
#define id_game_numlock 65
#define id_game_color 66
#define id_game_brightness 67

enum paint_tool { TOOL_NONE = 0, TOOL_CLEAR_ALL, TOOL_FILL_ALL };

static uint8_t flag_get(uint8_t field, uint8_t bit) {
    return (field & bit) ? 1 : 0;
}

static void flag_set(uint8_t *field, uint8_t bit, uint8_t on) {
    if (on) {
        *field |= bit;
    } else {
        *field &= ~bit;
    }
}

static void isycat_get(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_zone: v[0] = cfg.zone; break;
        case id_ind_enabled: v[0] = cfg.ind_enabled; break;
        case id_ind_color: v[0] = cfg.ind_hue; v[1] = cfg.ind_sat; break;
        case id_ind_brightness: v[0] = cfg.ind_val; break;
        case id_nav_pgupdn: v[0] = flag_get(cfg.nav_flags, NAV_PGUPDN); break;
        case id_nav_ctrl_arrows: v[0] = flag_get(cfg.nav_flags, NAV_CTRL_ARR); break;
        case id_nav_alt_arrows: v[0] = flag_get(cfg.nav_flags, NAV_ALT_ARR); break;
        case id_nav_tab_arrows: v[0] = flag_get(cfg.nav_flags, NAV_TAB_ARR); break;
        case id_swe_enabled: v[0] = flag_get(cfg.swe_flags, SWE_ON); break;
        case id_swe_plain_ralt: v[0] = flag_get(cfg.swe_flags, SWE_PLAIN_RALT); break;
        case id_swe_alt_keys: v[0] = flag_get(cfg.swe_flags, SWE_ALT_KEYS); break;
        case id_ug_mode: v[0] = cfg.ug_mode; break;
        case id_ug_color: v[0] = cfg.ug_hue; v[1] = cfg.ug_sat; break;
        case id_ug_brightness: v[0] = cfg.ug_val; break;
        case id_paint_mode: v[0] = paint_mode; break;
        case id_paint_brush: v[0] = paint_brush; break;
        case id_paint_brush_color:
            if (paint_brush) {
                v[0] = cfg.palette[paint_brush - 1][0];
                v[1] = cfg.palette[paint_brush - 1][1];
            } else {
                v[0] = 0;
                v[1] = 0;
            }
            break;
        case id_paint_on: v[0] = cfg.paint_on; break;
        case id_paint_tools: v[0] = TOOL_NONE; break;
        case id_palette_val_first ... id_palette_val_first + PALETTE_SIZE - 1:
            v[0] = cfg.palette[id - id_palette_val_first][2];
            break;
        case id_th_tap_first ... id_th_tap_first + TH_COUNT - 1:
            v[0] = cfg.th_tap[id - id_th_tap_first] >> 8;
            v[1] = cfg.th_tap[id - id_th_tap_first] & 0xFF;
            break;
        case id_th_hold_first ... id_th_hold_first + TH_COUNT - 1:
            v[0] = cfg.th_hold[id - id_th_hold_first] >> 8;
            v[1] = cfg.th_hold[id - id_th_hold_first] & 0xFF;
            break;
        case id_th_term: v[0] = cfg.th_term_10ms; break;
        case id_game_mode: v[0] = game_mode; break;
        case id_game_f1: v[0] = flag_get(cfg.game_flags, GAME_F1); break;
        case id_game_f1_target: v[0] = cfg.f1_target >> 8; v[1] = cfg.f1_target & 0xFF; break;
        case id_game_bypass: v[0] = flag_get(cfg.game_flags, GAME_BYPASS); break;
        case id_game_led: v[0] = flag_get(cfg.game_flags, GAME_LED); break;
        case id_game_numlock: v[0] = flag_get(cfg.game_flags, GAME_NUMLOCK); break;
        case id_game_color: v[0] = cfg.game_hue; v[1] = cfg.game_sat; break;
        case id_game_brightness: v[0] = cfg.game_val; break;
        case id_palette_first ... id_palette_first + PALETTE_SIZE - 1:
            v[0] = cfg.palette[id - id_palette_first][0];
            v[1] = cfg.palette[id - id_palette_first][1];
            break;
        case id_td_term: v[0] = cfg.td_term_10ms; break;
    }
}

static void isycat_set(uint8_t id, uint8_t *v) {
    switch (id) {
        case id_zone: zone_set(v[0], false); break;
        case id_ind_enabled: cfg.ind_enabled = v[0] ? 1 : 0; break;
        case id_ind_color: cfg.ind_hue = v[0]; cfg.ind_sat = v[1]; break;
        case id_ind_brightness: cfg.ind_val = v[0]; break;
        case id_nav_pgupdn: flag_set(&cfg.nav_flags, NAV_PGUPDN, v[0]); break;
        case id_nav_ctrl_arrows: flag_set(&cfg.nav_flags, NAV_CTRL_ARR, v[0]); break;
        case id_nav_alt_arrows: flag_set(&cfg.nav_flags, NAV_ALT_ARR, v[0]); break;
        case id_nav_tab_arrows: flag_set(&cfg.nav_flags, NAV_TAB_ARR, v[0]); break;
        case id_swe_enabled: flag_set(&cfg.swe_flags, SWE_ON, v[0]); break;
        case id_swe_plain_ralt: flag_set(&cfg.swe_flags, SWE_PLAIN_RALT, v[0]); break;
        case id_swe_alt_keys: flag_set(&cfg.swe_flags, SWE_ALT_KEYS, v[0]); break;
        case id_ug_mode: cfg.ug_mode = v[0] < UG_COUNT ? v[0] : UG_FOLLOW; break;
        case id_ug_color: cfg.ug_hue = v[0]; cfg.ug_sat = v[1]; break;
        case id_ug_brightness: cfg.ug_val = v[0]; break;
        case id_paint_mode:
            paint_mode  = v[0] ? true : false;
            paint_timer = timer_read();
            break;
        case id_paint_brush: paint_brush = v[0] <= PALETTE_SIZE ? v[0] : 0; break;
        case id_paint_brush_color:
            if (paint_brush) {
                cfg.palette[paint_brush - 1][0] = v[0];
                cfg.palette[paint_brush - 1][1] = v[1];
            }
            break;
        case id_paint_on: cfg.paint_on = v[0] ? 1 : 0; break;
        case id_paint_tools:
            if (v[0] == TOOL_CLEAR_ALL) paint_all(0);
            if (v[0] == TOOL_FILL_ALL) paint_all(paint_brush);
            cfg_save();
            break;
        case id_palette_val_first ... id_palette_val_first + PALETTE_SIZE - 1:
            cfg.palette[id - id_palette_val_first][2] = v[0];
            break;
        case id_th_tap_first ... id_th_tap_first + TH_COUNT - 1:
            cfg.th_tap[id - id_th_tap_first] = (v[0] << 8) | v[1];
            break;
        case id_th_hold_first ... id_th_hold_first + TH_COUNT - 1:
            cfg.th_hold[id - id_th_hold_first] = (v[0] << 8) | v[1];
            break;
        case id_th_term: cfg.th_term_10ms = v[0] < 5 ? 5 : v[0]; break;
        case id_game_mode:
            if ((v[0] ? true : false) != game_mode) game_mode_toggle();
            break;
        case id_game_f1: flag_set(&cfg.game_flags, GAME_F1, v[0]); break;
        case id_game_f1_target: cfg.f1_target = (v[0] << 8) | v[1]; break;
        case id_game_bypass: flag_set(&cfg.game_flags, GAME_BYPASS, v[0]); break;
        case id_game_led: flag_set(&cfg.game_flags, GAME_LED, v[0]); break;
        case id_game_numlock: flag_set(&cfg.game_flags, GAME_NUMLOCK, v[0]); break;
        case id_game_color: cfg.game_hue = v[0]; cfg.game_sat = v[1]; break;
        case id_game_brightness: cfg.game_val = v[0]; break;
        case id_palette_first ... id_palette_first + PALETTE_SIZE - 1:
            cfg.palette[id - id_palette_first][0] = v[0];
            cfg.palette[id - id_palette_first][1] = v[1];
            break;
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
