# Rasterize ASCII + Arabic (presentation forms) + Arabic-Indic digits from a
# TTF into a packed C header defining one `const AFont <name>`.
# Usage: generate_font.py <ttf> <size> <name> <out.h>
import sys
from PIL import Image, ImageFont, ImageDraw

FONT = sys.argv[1]
SIZE = int(sys.argv[2])
NAME = sys.argv[3]
OUT  = sys.argv[4]

font = ImageFont.truetype(FONT, SIZE)
ascent, descent = font.getmetrics()
line_h = ascent + descent

cps = list(range(0x20, 0x7F))
cps += list(range(0x0660, 0x066D))
cps += [0x060C, 0x061B, 0x061F]
cps += list(range(0xFE70, 0xFEFD))

PAD = 6
glyphs = []
blob = []

def render(cp):
    ch = chr(cp)
    adv = round(font.getlength(ch))
    img = Image.new("L", (adv + 2 * PAD, line_h + 2 * PAD), 0)
    ImageDraw.Draw(img).text((PAD, PAD + ascent), ch, font=font, fill=255, anchor="ls")
    bbox = img.getbbox()
    if bbox is None:
        return adv, 0, 0, 0, 0, []
    l, t, r, b = bbox
    crop = img.crop(bbox)
    nib = [p >> 4 for p in crop.getdata()]
    return adv, crop.width, crop.height, l - PAD, t - PAD, nib

for cp in cps:
    adv, w, h, xo, yo, nib = render(cp)
    off = len(blob)
    for i in range(0, len(nib), 2):
        hi = nib[i]
        lo = nib[i + 1] if i + 1 < len(nib) else 0
        blob.append((hi << 4) | lo)
    glyphs.append((cp, w, h, adv, xo, yo, off if (w and h) else 0))

space_adv = round(font.getlength(" "))

L = []
L.append("#pragma once")
L.append(f"// AUTO-GENERATED  font={FONT.split(chr(92))[-1]} size={SIZE} glyphs={len(glyphs)}")
L.append('#include "ui/ArabicFontDef.h"')
L.append("")
L.append("namespace txt {")
L.append(f"static const AGlyph {NAME}_g[{len(glyphs)}] = {{")
row = ""
for (cp, w, h, adv, xo, yo, off) in glyphs:
    row += f"{{0x{cp:04X},{w},{h},{adv},{xo},{yo},{off}}},"
    if len(row) > 100:
        L.append("  " + row); row = ""
if row: L.append("  " + row)
L.append("};")
L.append(f"static const uint8_t {NAME}_b[{len(blob)}] = {{")
row = ""
for b in blob:
    row += f"{b},"
    if len(row) > 112:
        L.append(row); row = ""
if row: L.append(row)
L.append("};")
L.append(f"const AFont {NAME} = {{{line_h},{ascent},{space_adv},{len(glyphs)},{NAME}_g,{NAME}_b}};")
L.append("}  // namespace txt")
L.append("")
open(OUT, "w", encoding="utf-8").write("\n".join(L))
print(f"{NAME}: glyphs={len(glyphs)} bytes={len(blob)} line_h={line_h} ascent={ascent}")
