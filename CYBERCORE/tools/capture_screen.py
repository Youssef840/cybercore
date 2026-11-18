import serial, time, sys
from pathlib import Path
from PIL import Image

port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
out  = sys.argv[2] if len(sys.argv) > 2 else "shot.png"
reset = "--reset" in sys.argv

s = serial.Serial()
s.port = port; s.baudrate = 115200; s.timeout = 2
s.dtr = False; s.rts = False
s.open()
if reset:
    s.dtr = False; s.rts = True; time.sleep(0.12); s.rts = False
    time.sleep(5)          # let it boot to the menu
else:
    s.dtr = False; s.rts = False
    time.sleep(0.3)
s.reset_input_buffer()
s.write(b"S")

# find header line "SHOT w h"
hdr = b""
t = time.time()
while time.time() - t < 6:
    line = s.readline()
    if line.startswith(b"SHOT"):
        hdr = line.strip(); break
if not hdr:
    print("no SHOT header; got:", line[:60]); sys.exit(1)
_, w, h = hdr.split()
w, h = int(w), int(h)
need = w * h * 2
buf = bytearray()
t = time.time()
while len(buf) < need and time.time() - t < 20:
    buf += s.read(need - len(buf))
print(f"got {len(buf)}/{need} bytes for {w}x{h}")
s.close()

if len(buf) != need:
    raise RuntimeError("Incomplete framebuffer")
Path(out).parent.mkdir(parents=True, exist_ok=True)
img = Image.new("RGB", (w, h))
px = img.load()
for i in range(min(len(buf) // 2, w * h)):
    v = (buf[2 * i] << 8) | buf[2 * i + 1]      # byte-swapped RGB565 (this panel)
    r = (v >> 11) & 0x1F; g = (v >> 5) & 0x3F; b = v & 0x1F
    px[i % w, i // w] = ((r * 255) // 31, (g * 255) // 63, (b * 255) // 31)
img = img.resize((w * 2, h * 2), Image.NEAREST)
img.save(out)
print("saved", out)
