"""Build via/ak820_pro_isycat.json (VIA v3 draft definition).

Usage: python3 tools/gen_via_definition.py [../../via.json]
The base file (fpb's board definition, keyboards/a_jazz/ak820pro/via.json) supplies the
physical layout and the board's own custom keycodes; everything else is defined here.
Value ids must match `enum via_isycat_value` in keymap.c (custom channel 0).
"""
import json, os, sys

here = os.path.dirname(os.path.abspath(__file__))
base = json.load(open(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "..", "via.json")))

# Must match the enabled effects in this build (keymap.c static-asserts the count).
EFFECTS = """SOLID_COLOR ALPHAS_MODS GRADIENT_UP_DOWN GRADIENT_LEFT_RIGHT BREATHING BAND_SAT BAND_VAL
BAND_PINWHEEL_SAT BAND_PINWHEEL_VAL BAND_SPIRAL_SAT BAND_SPIRAL_VAL CYCLE_ALL CYCLE_LEFT_RIGHT
CYCLE_UP_DOWN RAINBOW_MOVING_CHEVRON CYCLE_OUT_IN CYCLE_OUT_IN_DUAL CYCLE_PINWHEEL CYCLE_SPIRAL
DUAL_BEACON RAINBOW_BEACON RAINBOW_PINWHEELS RAINDROPS JELLYBEAN_RAINDROPS HUE_BREATHING
HUE_PENDULUM HUE_WAVE PIXEL_RAIN PIXEL_FLOW PIXEL_FRACTAL TYPING_HEATMAP DIGITAL_RAIN
SOLID_REACTIVE_SIMPLE SOLID_REACTIVE SOLID_REACTIVE_WIDE SOLID_REACTIVE_MULTIWIDE
SOLID_REACTIVE_CROSS SOLID_REACTIVE_MULTICROSS SOLID_REACTIVE_NEXUS SOLID_REACTIVE_MULTINEXUS
SPLASH MULTISPLASH SOLID_SPLASH SOLID_MULTISPLASH""".split()
assert len(EFFECTS) == 44

effect_opts = [["All Off", 0]] + [[e.replace("_", " ").title(), i + 1] for i, e in enumerate(EFFECTS)]
on = "{id_qmk_rgb_matrix_effect} != 0"

def toggle(label, name, vid, show_if=None):
    item = {"label": label, "type": "toggle", "content": [name, 0, vid]}
    if show_if:
        item = {"showIf": show_if, **item}
    return item

swe_on = "{id_isycat_swe_enabled} == 1"

definition = {
    "name": "AJAZZ AK820 Pro (isycat)",
    "vendorId": base["vendorId"],
    "productId": base["productId"],
    "matrix": base["matrix"],
    "customKeycodes": base["customKeycodes"] + [
        {"name": "Play / Next", "title": "Tap: Play/Pause. Double tap: Next track", "shortName": "PP_TD"},
        {"name": "AltGr SE", "title": "Right Alt with Swedish letters (see Tweaks menu)", "shortName": "AltGr SE"},
        {"name": "LED Zones", "title": "Lights on/off (Scroll Lock and game mode lights keep working)", "shortName": "LED Zone"},
    ] + [
        {"name": f"Tap/Hold {i}", "title": f"Tap/Hold {i}: tap and hold keycodes are set in Tweaks", "shortName": f"TH {i}"}
        for i in range(1, 5)
    ] + [
        {"name": "Game mode", "title": "Toggle game mode (F1 stays F1, remaps off). With Num Lock sync it toggles Num Lock", "shortName": "Game"},
    ],
    "menus": [
        {"label": "Lighting", "content": [
            {"label": "Backlight", "content": [
                {"label": "Lights", "type": "dropdown",
                 "options": ["On", "Off"],
                 "content": ["id_isycat_zone", 0, 1]},
                {"label": "Brightness", "type": "range", "options": [0, 255],
                 "content": ["id_qmk_rgb_matrix_brightness", 3, 1]},
                {"label": "Effect", "type": "dropdown", "options": effect_opts,
                 "content": ["id_qmk_rgb_matrix_effect", 3, 2]},
                {"showIf": on, "label": "Effect Speed", "type": "range", "options": [0, 255],
                 "content": ["id_qmk_rgb_matrix_effect_speed", 3, 3]},
                {"showIf": on, "label": "Color", "type": "color",
                 "content": ["id_qmk_rgb_matrix_color", 3, 4]},
            ]},
            {"label": "Per-key colours", "content": [
                toggle("Show painted key colours", "id_isycat_paint_on", 19),
                {"label": "Brush", "type": "dropdown",
                 "options": ["Clear (show effect)"] + [f"Colour {i}" for i in range(1, 15)] + ["Colour 15 (default: off)"],
                 "content": ["id_isycat_paint_brush", 0, 17]},
                toggle("Paint mode: press keys on the keyboard to paint them (no typing; ends after 1 min idle)",
                       "id_isycat_paint_mode", 16),
                {"label": "Clear all painted keys", "type": "button", "options": [1],
                 "content": ["id_isycat_paint_clear", 0, 20]},
                {"label": "Fill all keys with the brush", "type": "button", "options": [2],
                 "content": ["id_isycat_paint_fill", 0, 20]},
            ]},
            {"label": "Colour palette", "content": [
                item
                for i in range(15)
                for item in (
                    {"label": f"Colour {i + 1}", "type": "color", "content": [f"id_isycat_palette_{i + 1}", 0, 21 + i]},
                    {"label": f"Colour {i + 1} brightness (0 = off)", "type": "range", "options": [0, 255],
                     "content": [f"id_isycat_palette_val_{i + 1}", 0, 36 + i]},
                )
            ]},
            {"label": "Sleep", "content": [
                {"label": "Lights and screen sleep after (minutes, 0 = never)", "type": "range", "options": [0, 60],
                 "content": ["id_isycat_sleep_minutes", 0, 68]},
                toggle("Stay awake while Scroll Lock is on (mic muted)", "id_isycat_sleep_scrl_awake", 69),
            ]},
            {"label": "Scroll Lock indicator", "content": [
                toggle("Show Scroll Lock on the key that has Scroll Lock (base layer, else Fn layer)", "id_isycat_ind_enabled", 2),
                {"showIf": "{id_isycat_ind_enabled} == 1", "label": "Color", "type": "color",
                 "content": ["id_isycat_ind_color", 0, 3]},
                {"showIf": "{id_isycat_ind_enabled} == 1", "label": "Brightness", "type": "range",
                 "options": [0, 255], "content": ["id_isycat_ind_brightness", 0, 4]},
            ]},
        ]},
        {"label": "Tweaks", "content": [
            {"label": "Navigation", "content": [
                toggle("Ctrl + PgUp/PgDn = Ctrl + Home/End", "id_isycat_nav_pgupdn", 5),
                toggle("Ctrl + [Shift] + Left/Right = [Shift] + Home/End", "id_isycat_nav_ctrl_arrows", 8),
                toggle("Alt + [Shift] + Left/Right = word jump (Ctrl + [Shift] + Left/Right)", "id_isycat_nav_alt_arrows", 9),
                toggle("Ctrl + Alt + Left/Right = previous/next tab", "id_isycat_nav_tab_arrows", 10),
            ]},
            {"label": "Swedish letters", "content": [
                toggle("AltGr SE key + a / o / e = å / ö / ä", "id_isycat_swe_enabled", 6),
                toggle("Also on a plain Right Alt key", "id_isycat_swe_plain_ralt", 11, swe_on),
                toggle("Also with [ ; ' (å ö ä)", "id_isycat_swe_alt_keys", 12, swe_on),
            ]},
            {"label": "Tap/Hold keys", "content": [
                item
                for i in range(4)
                for item in (
                    {"label": f"Tap/Hold {i + 1}: on tap", "type": "keycode", "content": [f"id_isycat_th_tap_{i + 1}", 0, 51 + i]},
                    {"label": f"Tap/Hold {i + 1}: while held", "type": "keycode", "content": [f"id_isycat_th_hold_{i + 1}", 0, 55 + i]},
                )
            ] + [
                {"label": "Hold after (x10 ms)", "type": "range", "options": [5, 100],
                 "content": ["id_isycat_th_term", 0, 59]},
            ]},
            {"label": "Game mode", "content": [
                toggle("Game mode on (now)", "id_isycat_game_mode", 60),
                toggle("Follow host Num Lock (AHK sets it in fullscreen apps)", "id_isycat_game_numlock", 65),
                toggle("F1 sends the key below outside game mode", "id_isycat_game_f1", 61),
                {"showIf": "{id_isycat_game_f1} == 1", "label": "F1 outside game mode", "type": "keycode",
                 "content": ["id_isycat_game_f1_target", 0, 62]},
                toggle("Game mode turns off nav remaps and Swedish letters", "id_isycat_game_bypass", 63),
                toggle("Light up F1 in game mode", "id_isycat_game_led", 64),
                {"showIf": "{id_isycat_game_led} == 1", "label": "F1 light colour", "type": "color",
                 "content": ["id_isycat_game_color", 0, 66]},
                {"showIf": "{id_isycat_game_led} == 1", "label": "F1 light brightness", "type": "range", "options": [0, 255],
                 "content": ["id_isycat_game_brightness", 0, 67]},
            ]},
            {"label": "Play / Next key", "content": [
                {"label": "Double-tap window (x10 ms)", "type": "range", "options": [10, 100],
                 "content": ["id_isycat_td_term", 0, 7]},
            ]},
        ]},
    ],
    "layouts": base["layouts"],
}

out = os.path.join(here, "..", "via", "ak820_pro_isycat.json")
json.dump(definition, open(out, "w"), indent=2, ensure_ascii=False)
print("wrote", os.path.normpath(out))
