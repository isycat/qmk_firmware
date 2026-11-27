/* AJAZZ / EPOMAKER AK820 Pro (a_jazz/ak820pro) VIA keymap for isycat.
 *
 * Port of the Drop CTRL v1 isycat keymap (massdrop/ctrl/keymaps/isycat):
 *   - Navigation remaps (Ctrl+PgUp/PgDn, Ctrl/Alt+[Shift]+Left/Right, Ctrl+Alt+Left/Right)
 *   - Play/Next tap dance (knob press by default): 1 tap = Play/Pause, 2 taps = Next track
 *   - AltGr Swedish letters: RAlt + a/[ = å, RAlt + o/; = ö, RAlt + e/' = ä (Shift = capital)
 *   - Scroll Lock state shown on the key that has Scroll Lock (Fn+Del by default)
 *   - Tap/Hold keys, game mode (F1 -> Alt+F23 outside games, Num Lock sync), per-key paint
 *   - Lights/screen idle sleep time set in VIA, and kept awake while Scroll Lock is on
 *   - All of the above configurable from VIA via a custom definition JSON (via/ folder)
 *
 * Board support (LCD, wireless, RGB driver, knob) is fpb's port; this file only adds keymap logic.
 * GPL-2.0-or-later.
 */
#include QMK_KEYBOARD_H
#include "ak820pro.h"
#include "graphics/lcd_bus.h"
#include "lib/lib8tion/lib8tion.h"
#include "keymap_introspection.h"
#include "connection.h"
#include "usb_main.h"
#include "deep_sleep.h"

// Board-level VIA handler in ak820pro.c (clock, LCD flash and now-playing channels)
void via_custom_value_command_kb(uint8_t *data, uint8_t length);

// Custom keycodes follow the board's own (SCR_TOG..SCR_MEDIA), so the VIA
// definition lists the board's 19 keycodes first and then these.
enum isycat_keycodes {
    CK_PPTD = AK820PRO_SAFE_RANGE, // Play/Pause on tap, Next track on double tap
    CK_SWALT,                      // Right Alt with Swedish letters (å ä ö)
    CK_LZONE,                      // Lights on/off (the Scroll Lock indicator keeps working)
    CK_TH1,                        // Tap/Hold 1..4: tap and hold keycodes set in VIA (Tweaks)
    CK_TH2,
    CK_TH3,
    CK_TH4,
    CK_GAME,                       // Toggle game mode (F1 stays F1, optional remap bypass)
};
_Static_assert((int)CK_GAME <= (int)QK_KB_31, "custom keycodes must stay in VIA's QK_KB range");

enum layer_names { WINBASE, WINFN, MACBASE, MACFN };

#define KC_TASK LGUI(KC_TAB)       // Task viewer
#define KC_FLXP LGUI(KC_E)         // Windows file explorer
#define KC_MCTL KC_MISSION_CONTROL // Mission Control

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Windows base: Caps = F24 (AHK), Win = Tap/Hold 1 (hold Win, tap Alt+F23),
     * Right Alt = AltGr SE, knob press = Play/Next. */
    [WINBASE] = LAYOUT_82_ansi(
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_DEL,   CK_PPTD,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,  KC_HOME,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,  KC_PGUP,
        KC_F24,   KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,   KC_PGDN,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL,  CK_TH1,   KC_LALT,                                KC_SPC,                       CK_SWALT, MO(WINFN),KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT
    ),
    /* Windows Fn: fpb's Fn layer, plus Fn+Del = Scroll Lock, Fn+G = Game, Fn+Bksp = Insert,
     * Fn+PgUp = Print Screen, Fn+PgDn = End, Fn+Enter = LCD animation, Fn+N = NKRO,
     * Fn+knob = Mute. */
    [WINFN] = LAYOUT_82_ansi(
        QK_BOOT,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FLXP,  _______,  _______,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_SCRL,  KC_MUTE,
        _______,  _______,  _______,  _______,  _______,  _______,  RM_SATU,  RM_SATD,  _______,  _______,  _______,  RM_SPDD,  RM_SPDU,  KC_INS,   SCR_TOG,
        _______,  BT1,      BT2,      BT3,      BT24G,    _______,  _______,  _______,  _______,  _______,  BT_PAIR,  _______,  _______,  RM_NEXT,  KC_PSCR,
        _______,  _______,  _______,  _______,  _______,  CK_GAME,  _______,  _______,  _______,  _______,  _______,  _______,            ANIM_TOG, KC_END,
        _______,            _______,  RM_TOGG,  _______,  _______,  _______,  NK_TOGG,  _______,  _______,  _______,  _______,  _______,  RM_VALU,
        _______,  GU_TOGG,  _______,                                _______,                      _______,  _______,  _______,  RM_HUED,  RM_VALD,  RM_HUEU
    ),
    // Mac layers: unchanged from fpb's VIA keymap (used when the switch is on Mac).
    [MACBASE] = LAYOUT_82_ansi(
        KC_ESC,   KC_BRID,  KC_BRIU,  KC_MCTL,  KC_F4,    KC_F5,    KC_F6,    KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_DEL,   KC_MUTE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,  KC_HOME,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,  KC_PGUP,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,   KC_PGDN,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL,  KC_LALT,  KC_LGUI,                                KC_SPC,                       KC_RGUI,  MO(MACFN),KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT
    ),
    [MACFN] = LAYOUT_82_ansi(
        QK_BOOT,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   ANIM_TOG, KC_MUTE,
        _______,  _______,  _______,  _______,  _______,  _______,  RM_SATU,  RM_SATD,  _______,  _______,  _______,  RM_SPDD,  RM_SPDU,  _______,  SCR_TOG,
        _______,  BT1,      BT2,      BT3,      BT24G,    _______,  _______,  _______,  _______,  _______,  BT_PAIR,  _______,  _______,  RM_NEXT,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,
        _______,            _______,  RM_TOGG,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  RM_VALU,
        _______,  _______,  _______,                                _______,                      _______,  _______,  _______,  RM_HUED,  RM_VALD,  RM_HUEU
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [WINBASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WINFN]   = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MACBASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MACFN]   = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
};
#endif
// clang-format on

/* ------------------------------------------------------------------------- */
/* Persistent settings (EEPROM user datablock)                               */
/* ------------------------------------------------------------------------- */

#define KEY_LED_COUNT RGB_MATRIX_LED_COUNT // 81, every LED sits under a key
#define PALETTE_SIZE 15
#define TH_COUNT 4

enum game_flag {
    GAME_F1      = 1 << 0, // F1 sends f1_target outside game mode
    GAME_BYPASS  = 1 << 1, // game mode switches off nav remaps and Swedish letters
    GAME_LED     = 1 << 2, // show game mode on the F1 key LED
    GAME_NUMLOCK = 1 << 3, // game mode follows the host's Num Lock (set by AHK)
};

enum sleep_flag {
    SLEEP_SCRL_AWAKE = 1 << 0, // don't sleep the lights/screen while Scroll Lock is on
};

enum lighting_zone { ZONE_ALL = 0, ZONE_OFF, ZONE_COUNT };

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  zone;         // enum lighting_zone
    uint8_t  ind_enabled;  // Scroll Lock indicator on/off
    uint8_t  ind_hue;      // Scroll Lock indicator colour
    uint8_t  ind_sat;
    uint8_t  ind_val;      // Scroll Lock indicator brightness
    uint8_t  nav_flags;    // NAV_* bits: which navigation remaps are on
    uint8_t  swe_flags;    // SWE_* bits: Swedish letter options
    uint8_t  td_term_10ms; // Play/Next double-tap window in 10 ms units
    uint8_t  paint_on;     // show painted per-key colours
    uint8_t  th_term_10ms; // Tap/Hold keys: hold threshold in 10 ms units
    uint8_t  game_flags;   // GAME_* bits
    uint8_t  game_hue;     // game mode F1 LED colour
    uint8_t  game_sat;
    uint8_t  game_val;     // game mode F1 LED brightness
    uint8_t  sleep_min;    // lights/screen sleep after this many idle minutes, 0 = never
    uint8_t  sleep_flags;  // SLEEP_* bits
    uint8_t  palette[PALETTE_SIZE][3];       // hue, sat, brightness of brush colours 1..15
    uint8_t  keymap[(KEY_LED_COUNT + 1) / 2]; // 4 bits per key LED: 0 = effect, n = palette[n-1]
    uint16_t th_tap[TH_COUNT];  // Tap/Hold keys: keycode sent on tap
    uint16_t th_hold[TH_COUNT]; // Tap/Hold keys: keycode held on hold
    uint16_t f1_target;         // what F1 sends outside game mode
    uint8_t  deep_min;          // MCU deep sleep on battery after this many idle minutes, 0 = off
    uint8_t  future[38];        // headroom so later settings don't move VIA's data
} user_config_t;

_Static_assert(sizeof(user_config_t) == EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE mismatch");

#define USER_CONFIG_MAGIC 0xA8

enum nav_flag {
    NAV_PGUPDN   = 1 << 0, // Ctrl+PgUp/PgDn -> Ctrl+Home/End
    NAV_CTRL_ARR = 1 << 1, // Ctrl+[Shift]+Left/Right -> [Shift]+Home/End
    NAV_ALT_ARR  = 1 << 2, // Alt+[Shift]+Left/Right -> Ctrl+[Shift]+Left/Right
    NAV_TAB_ARR  = 1 << 3, // Ctrl+Alt+Left/Right -> Ctrl+[Shift]+Tab
    NAV_ALL      = NAV_PGUPDN | NAV_CTRL_ARR | NAV_ALT_ARR | NAV_TAB_ARR,
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
        .paint_on     = 1,
        .th_term_10ms = 20,
        .game_flags   = GAME_F1 | GAME_LED | GAME_NUMLOCK,
        .game_hue     = 85,
        .game_sat     = 255,
        .game_val     = 255,
        .sleep_min    = 3,
        .sleep_flags  = SLEEP_SCRL_AWAKE,
        .palette = {
            {0, 255, 255}, {21, 255, 255}, {43, 255, 255}, {64, 255, 255}, {85, 255, 255},
            {106, 255, 255}, {128, 255, 255}, {149, 255, 255}, {170, 255, 255}, {191, 255, 255},
            {213, 255, 255}, {234, 255, 255}, {0, 0, 255}, {28, 96, 255},
            {0, 0, 0}, // colour 15: off
        },
        // Tap/Hold 1: hold = Win, tap = Alt+F23 (PowerToys Command Palette)
        .th_tap    = {LALT(KC_F23), KC_NO, KC_NO, KC_NO},
        .th_hold   = {KC_LGUI, KC_NO, KC_NO, KC_NO},
        .f1_target = LALT(KC_F23),
    };
}

/* EEPROM lives in the MCU's own flash on this board, and a flash write stalls the
 * core. Writing while an LCD DMA transfer runs can wedge it, so saves are
 * collected and written once things settle and no transfer is running. */
#define CFG_SETTLE_MS 500

static bool     cfg_dirty;
static uint16_t cfg_dirty_timer;

static void cfg_save(void) {
    cfg_dirty       = true;
    cfg_dirty_timer = timer_read();
}

static void cfg_flush_task(void) {
    if (cfg_dirty && timer_elapsed(cfg_dirty_timer) >= CFG_SETTLE_MS && !lcd_blit_busy()) {
        cfg_dirty = false;
        eeconfig_update_user_datablock(&cfg, 0, sizeof(cfg));
    }
}

void via_init_kb(void) {
    user_config_t probe;
    eeconfig_read_user_datablock(&probe, 0, sizeof(probe));
    if (!eeconfig_is_user_datablock_valid() || probe.magic != USER_CONFIG_MAGIC) {
        via_eeprom_set_valid(false);
    }
}

void eeconfig_init_user(void) {
    cfg_defaults();
    eeconfig_update_user_datablock(&cfg, 0, sizeof(cfg));
}

/* ------------------------------------------------------------------------- */
/* Lights on/off, Scroll Lock and game mode indicators                       */
/* ------------------------------------------------------------------------- */

// The VIA definition (via/ak820_pro_isycat.json) lists effects by index.
_Static_assert(RGB_MATRIX_EFFECT_MAX == 45, "RGB effect list changed: update the VIA definition JSON");

static void zone_apply(void) {
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

/* The Scroll Lock light goes on whichever key has Scroll Lock in the Windows base
 * layer, or in the Fn layer if the base layer has none. The game mode light goes
 * on the key(s) that have F1 in the base layer. Rescanned every second, so
 * moving the keys in VIA moves the lights. */
#define LED_BITS ((KEY_LED_COUNT + 7) / 8)

static uint8_t  scrl_leds[LED_BITS];
static uint8_t  f1_leds[LED_BITS];
static uint16_t led_scan_timer;
static bool     led_scan_done;

static bool led_bit(const uint8_t *bits, uint8_t led) {
    return bits[led / 8] & (1 << (led % 8));
}

static bool scan_layer(uint8_t layer, uint16_t kc, uint8_t *bits) {
    bool found = false;
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            uint8_t led = g_led_config.matrix_co[r][c];
            if (led >= KEY_LED_COUNT) continue;
            if (keycode_at_keymap_location(layer, r, c) == kc) {
                bits[led / 8] |= 1 << (led % 8);
                found = true;
            }
        }
    }
    return found;
}

static void led_scan_task(void) {
    if (led_scan_done && timer_elapsed(led_scan_timer) < 1000) return;
    led_scan_done  = true;
    led_scan_timer = timer_read();
    memset(scrl_leds, 0, sizeof(scrl_leds));
    memset(f1_leds, 0, sizeof(f1_leds));
    if (!scan_layer(WINBASE, KC_SCRL, scrl_leds)) scan_layer(WINFN, KC_SCRL, scrl_leds);
    scan_layer(WINBASE, KC_F1, f1_leds);
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
    *b         = (led & 1) ? ((*b & 0x0F) | (c << 4)) : ((*b & 0xF0) | (c & 0x0F));
}

static void paint_all(uint8_t c) {
    for (uint8_t i = 0; i < KEY_LED_COUNT; i++) key_color_set(i, c);
}

static void paint_task(void) {
    if (paint_mode && timer_elapsed(paint_timer) > PAINT_TIMEOUT_MS) paint_mode = false;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    bool keys_visible = cfg.zone == ZONE_ALL;

    for (uint8_t i = led_min; i < led_max && i < KEY_LED_COUNT; i++) {
        // Painted keys are their own layer: they keep their own colour and
        // brightness even when the effect is off or dark.
        uint8_t c = cfg.paint_on || paint_mode ? key_color(i) : 0;
        if (c) {
            const uint8_t *pc  = cfg.palette[c - 1];
            rgb_t          rgb = hsv_to_rgb((hsv_t){pc[0], pc[1], pc[2]});
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        } else if (!keys_visible) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }
    bool game_on = game_mode && (cfg.game_flags & GAME_LED);
    bool scrl_on = cfg.ind_enabled && host_keyboard_led_state().scroll_lock;
    if (game_on || scrl_on) {
        rgb_t game = hsv_to_rgb((hsv_t){cfg.game_hue, cfg.game_sat, cfg.game_val});
        rgb_t scrl = hsv_to_rgb((hsv_t){cfg.ind_hue, cfg.ind_sat, cfg.ind_val});
        for (uint8_t i = led_min; i < led_max && i < KEY_LED_COUNT; i++) {
            if (scrl_on && led_bit(scrl_leds, i)) {
                rgb_matrix_set_color(i, scrl.r, scrl.g, scrl.b);
            } else if (game_on && led_bit(f1_leds, i)) {
                rgb_matrix_set_color(i, game.r, game.g, game.b);
            }
        }
    }
    return false;
}

/* Lights and screen sleep after the idle time set in VIA, except while Scroll
 * Lock is on (mic muted) if that option is set, so the mute light stays visible. */
static bool stay_awake(void) {
    return (cfg.sleep_flags & SLEEP_SCRL_AWAKE) && cfg.ind_enabled && host_keyboard_led_state().scroll_lock;
}

static bool on_battery(void) {
    return connection_get_host() != CONNECTION_HOST_USB && USB_DRIVER.state != USB_ACTIVE;
}

uint32_t display_sleep_timeout_ms(void) {
    if (stay_awake()) return 0;
    uint32_t ms = (uint32_t)cfg.sleep_min * 60000u;
    // Deep sleep needs the lights and screen off first, so they never stay on longer than that.
    uint32_t deep = (uint32_t)cfg.deep_min * 60000u;
    if (deep && on_battery() && (ms == 0 || deep < ms)) ms = deep;
    return ms;
}


/* ------------------------------------------------------------------------- */
/* Play/Next tap dance                                                       */
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
    uint8_t saved       = get_mods();
    uint8_t weak        = get_weak_mods();
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
    uint8_t                  letter        = idx / 2;
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

bool dip_switch_update_user(uint8_t index, bool active) {
    // Mac/Windows switch picks the base layer. The wireless slider and the
    // screen icons are handled by the board code (dip_switch_update_kb).
    if (index == 0) {
        if (active) {
            set_single_persistent_default_layer(WINBASE);
        } else {
            set_single_persistent_default_layer(MACBASE);
            keymap_config.no_gui = false;
        }
    }
    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    bool pressed = record->event.pressed;

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
    // survives loading a VIA layout that still has KC_RALT there.
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
        // Every "lights on/off" key switches the lights off without stopping the
        // matrix, so the Scroll Lock and game mode lights keep working.
        case CK_LZONE:
        case RGBM_TOG:
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
        // Mac media keys (from fpb's keymap)
        case KC_MISSION_CONTROL:
            host_consumer_send(pressed ? 0x29F : 0);
            return false;
        case KC_LAUNCHPAD:
            host_consumer_send(pressed ? 0x2A0 : 0);
            return false;
        default:
            return true;
    }
}

/* Deep sleep (see deep_sleep.c): on battery only, after the lights and screen
 * have gone dark. A wake-up that brings no key press lets it sleep again 10 s
 * later; the clock stands still while asleep, so idle time doesn't grow. */
static bool     deep_woke;
static uint32_t deep_wake_timer;

static void deep_sleep_task(void) {
    if (!cfg.deep_min || stay_awake() || !on_battery()) return;
    if (last_input_activity_elapsed() < (uint32_t)cfg.deep_min * 60000u + 2000u) return;
    if (!rgb_matrix_get_suspend_state()) return; // lights not blanked yet
    if (deep_woke && timer_elapsed32(deep_wake_timer) < 10000) return;
    if (cfg_dirty || lcd_blit_busy() || pp_taps || th_state != TH_IDLE) return;
    deep_sleep_enter();
    deep_woke       = true;
    deep_wake_timer = timer_read32();
}

void housekeeping_task_user(void) {
    pp_task();
    th_task();
    paint_task();
    numlock_sync_task();
    led_scan_task();
    cfg_flush_task();
    deep_sleep_task();
}

void keyboard_post_init_user(void) {
    eeconfig_read_user_datablock(&cfg, 0, sizeof(cfg));
    if (!eeconfig_is_user_datablock_valid() || cfg.magic != USER_CONFIG_MAGIC) {
        eeconfig_init_user();
    }
    zone_apply();
}

/* ------------------------------------------------------------------------- */
/* VIA custom menu (channel 0)                                               */
/* ------------------------------------------------------------------------- */
/* Value ids must match via/ak820_pro_isycat.json. They are the same as the
 * Drop CTRL keymap's where the setting exists on both boards. */
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
    // 13..15 are the CTRL's underglow settings; this board has no underglow
    id_paint_mode = 16,
    id_paint_brush,
    id_paint_brush_color,
    id_paint_on,
    id_paint_tools,
};

#define id_palette_first 21     // ids 21..35 = palette colours 1..15 (hue, sat)
#define id_palette_val_first 36 // ids 36..50 = palette brightness 1..15
#define id_th_tap_first 51      // ids 51..54 = Tap/Hold tap keycodes
#define id_th_hold_first 55     // ids 55..58 = Tap/Hold hold keycodes
#define id_th_term 59
#define id_game_mode 60 // not saved
#define id_game_f1 61
#define id_game_f1_target 62
#define id_game_bypass 63
#define id_game_led 64
#define id_game_numlock 65
#define id_game_color 66
#define id_game_brightness 67
#define id_sleep_minutes 68
#define id_sleep_scrl_awake 69
#define id_deep_sleep_minutes 70

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
        case id_paint_mode: v[0] = paint_mode; break;
        case id_paint_brush: v[0] = paint_brush; break;
        case id_paint_brush_color:
            v[0] = paint_brush ? cfg.palette[paint_brush - 1][0] : 0;
            v[1] = paint_brush ? cfg.palette[paint_brush - 1][1] : 0;
            break;
        case id_paint_on: v[0] = cfg.paint_on; break;
        case id_paint_tools: v[0] = TOOL_NONE; break;
        case id_palette_first ... id_palette_first + PALETTE_SIZE - 1:
            v[0] = cfg.palette[id - id_palette_first][0];
            v[1] = cfg.palette[id - id_palette_first][1];
            break;
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
        case id_sleep_minutes: v[0] = cfg.sleep_min; break;
        case id_sleep_scrl_awake: v[0] = flag_get(cfg.sleep_flags, SLEEP_SCRL_AWAKE); break;
        case id_deep_sleep_minutes: v[0] = cfg.deep_min; break;
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
        case id_palette_first ... id_palette_first + PALETTE_SIZE - 1:
            cfg.palette[id - id_palette_first][0] = v[0];
            cfg.palette[id - id_palette_first][1] = v[1];
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
        case id_sleep_minutes: cfg.sleep_min = v[0] > 60 ? 60 : v[0]; break;
        case id_sleep_scrl_awake: flag_set(&cfg.sleep_flags, SLEEP_SCRL_AWAKE, v[0]); break;
        case id_deep_sleep_minutes: cfg.deep_min = v[0] > 120 ? 120 : v[0]; break;
        case id_td_term: cfg.td_term_10ms = v[0] < 10 ? 10 : v[0]; break;
    }
}

static void isycat_command(uint8_t *data) {
    // data = [ command_id, channel_id, value_id, value_data... ]
    uint8_t *command_id = &data[0];
    uint8_t *value_id   = &data[2];
    uint8_t *value_data = &data[3];
    switch (*command_id) {
        case id_custom_set_value: isycat_set(*value_id, value_data); break;
        case id_custom_get_value: isycat_get(*value_id, value_data); break;
        case id_custom_save: cfg_save(); break;
        default: *command_id = id_unhandled; break;
    }
}

/* VIA dispatch. Channel 0 is this keymap's menu. VIA's built-in RGB matrix
 * channel turns the whole matrix off for effect 0, which would also kill the
 * Scroll Lock indicator, so that is mapped to "lights off" instead. Everything
 * else (the clock, LCD flash and now-playing channels) goes to the board code. */
void via_custom_value_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &data[0];
    uint8_t *channel_id = &data[1];
    uint8_t *value_id   = &data[2];
    uint8_t *value_data = &data[3];

    if (*channel_id == id_custom_channel) {
        isycat_command(data);
        return;
    }
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
    }
    if (*channel_id == id_qmk_rgb_matrix_channel) {
        if (*command_id == id_custom_save) {
            // Write only when no LCD transfer is running (see cfg_save).
            for (uint16_t t = timer_read(); lcd_blit_busy() && timer_elapsed(t) < 150;) {
            }
            eeconfig_update_rgb_matrix(&rgb_matrix_config);
            cfg_save();
            return;
        }
        via_qmk_rgb_matrix_command(data, length);
        return;
    }
    via_custom_value_command_kb(data, length);
}
