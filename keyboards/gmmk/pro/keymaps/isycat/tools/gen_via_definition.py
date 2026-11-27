"""Build via/gmmk_pro_isycat.json (VIA v3 draft definition) for every GMMK Pro variant.

Usage: python3 tools/gen_via_definition.py <the-via/keyboards>/v3/gmmk/pro/gmmk_pro.json
The base file supplies the physical layout; everything else is defined here.
Value ids must match `enum via_isycat_value` in keymap.c (custom channel 0).
"""
import json, os, sys

base = json.load(open(sys.argv[1]))
here = os.path.dirname(os.path.abspath(__file__))

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
    "name": "GMMK Pro (isycat)",
    "vendorId": base["vendorId"],
    "productId": base["productId"],
    "matrix": base["matrix"],
    "keycodes": ["qmk_lighting"],
    "customKeycodes": [
        {"name": "Play / Next", "title": "Tap: Play/Pause. Double tap: Next track", "shortName": "PP_TD"},
        {"name": "AltGr SE", "title": "Right Alt with Swedish letters (see Tweaks menu)", "shortName": "AltGr SE"},
        {"name": "LED Zones", "title": "Cycle lighting zones: all, keys only, side lights only, off", "shortName": "LED Zone"},
    ] + [
        {"name": f"Tap/Hold {i}", "title": f"Tap/Hold {i}: tap and hold keycodes are set in Tweaks", "shortName": f"TH {i}"}
        for i in range(1, 5)
    ],
    "menus": [
        {"label": "Lighting", "content": [
            {"label": "Backlight", "content": [
                {"label": "Zones", "type": "dropdown",
                 "options": ["All", "Keys only", "Side lights only", "Off"],
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
            {"label": "Side lights", "content": [
                {"label": "Side lights", "type": "dropdown",
                 "options": ["Same as keys", "Own colour", "Own colour, breathing", "Rainbow", "Off"],
                 "content": ["id_isycat_ug_mode", 0, 13]},
                {"showIf": "{id_isycat_ug_mode} == 1 || {id_isycat_ug_mode} == 2", "label": "Colour",
                 "type": "color", "content": ["id_isycat_ug_color", 0, 14]},
                {"showIf": "{id_isycat_ug_mode} != 0 && {id_isycat_ug_mode} != 4", "label": "Brightness",
                 "type": "range", "options": [0, 255], "content": ["id_isycat_ug_brightness", 0, 15]},
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
            {"label": "Scroll Lock indicator", "content": [
                toggle("Show Scroll Lock state", "id_isycat_ind_enabled", 2),
                {"showIf": "{id_isycat_ind_enabled} == 1", "label": "Show it on", "type": "dropdown",
                 "options": ["The Scroll Lock key (side lights if none)", "Side lights", "Both"],
                 "content": ["id_isycat_ind_where", 0, 60]},
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
            {"label": "Play / Next key", "content": [
                {"label": "Double-tap window (x10 ms)", "type": "range", "options": [10, 100],
                 "content": ["id_isycat_td_term", 0, 7]},
            ]},
        ]},
    ],
    "layouts": base["layouts"],
}

out = os.path.join(here, "..", "via", "gmmk_pro_isycat.json")
json.dump(definition, open(out, "w"), indent=2, ensure_ascii=False)
print("wrote", os.path.normpath(out))
