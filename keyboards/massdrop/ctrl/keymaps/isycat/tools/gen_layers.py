"""Convert a VIA layout export (matrix order) into raw QMK keymap arrays."""
import json, re, sys
src, out = sys.argv[1], sys.argv[2]
d = json.load(open(src))
REN = {"KC_SLCK": "KC_SCRL", "MAGIC_TOGGLE_NKRO": "NK_TOGG"}
# Firmware-specific replacements on the base layer: (layer,row,col) -> keycode
OVR = {(0, 6, 7): "CK_PPTD", (0, 5, 4): "CK_SWALT", (0, 5, 1): "CK_TH1", (1, 3, 5): "CK_GAME"}
def conv(k):
    k = REN.get(k, k)
    k = re.sub(r"CUSTOM\((\d+)\)", lambda m: f"QK_KB_{m.group(1)}", k)
    k = re.sub(r"^C\((.*)\)$", r"LCTL(\1)", k)
    return k
lines = ["// Generated from drop_ctrl.layout.json by tools/gen_layers.py. Edit in VIA, not here.",
         "// Matrix order: 11 rows x 8 cols (see keyboards/massdrop/ctrl/keyboard.json).",
         "const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {"]
for li, layer in enumerate(d["layers"]):
    assert len(layer) == 88, len(layer)
    lines.append(f"    [{li}] = {{")
    for r in range(11):
        row = [OVR.get((li, r, c), conv(layer[r * 8 + c])) for c in range(8)]
        lines.append("        { " + ", ".join(f"{k:>10}" for k in row) + " },")
    lines.append("    },")
lines.append("};")
open(out, "w").write("\n".join(lines) + "\n")
