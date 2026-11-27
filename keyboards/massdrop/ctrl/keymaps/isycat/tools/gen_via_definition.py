"""Build via/drop_ctrl_v1_isycat.json (VIA v3 draft definition).

Usage: python3 tools/gen_via_definition.py <the-via/keyboards>/v3/drop/ctrl/v1/ctrl-v1.json
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
    "name": "Drop CTRL v1 (isycat)",
    "vendorId": base["vendorId"],
    "productId": base["productId"],
    "matrix": base["matrix"],
    "keycodes": ["qmk_lighting"],
    "customKeycodes": base["customKeycodes"] + [
        {"name": "Play / Next", "title": "Tap: Play/Pause. Double tap: Next track", "shortName": "PP_TD"},
        {"name": "AltGr SE", "title": "Right Alt with Swedish letters (see Tweaks menu)", "shortName": "AltGr SE"},
        {"name": "LED Zones", "title": "Cycle lighting zones: all, keys only, underglow only, off", "shortName": "LED Zone"},
    ],
    "menus": [
        {"label": "Lighting", "content": [
            {"label": "Backlight", "content": [
                {"label": "Zones", "type": "dropdown",
                 "options": ["All", "Keys only", "Underglow only", "Off"],
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
            {"label": "Scroll Lock indicator", "content": [
                toggle("Show Scroll Lock on its key", "id_isycat_ind_enabled", 2),
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
            {"label": "Play / Next key", "content": [
                {"label": "Double-tap window (x10 ms)", "type": "range", "options": [10, 100],
                 "content": ["id_isycat_td_term", 0, 7]},
            ]},
        ]},
    ],
    "layouts": base["layouts"],
}

out = os.path.join(here, "..", "via", "drop_ctrl_v1_isycat.json")
json.dump(definition, open(out, "w"), indent=2, ensure_ascii=False)
print("wrote", os.path.normpath(out))
