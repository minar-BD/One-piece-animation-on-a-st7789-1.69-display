# h_to_bin.py
import re, struct, os

H_FILE   = "animation.h"
OUT_FILE = "data/gif.bin"
FRAME_W  = 240
FRAME_H  = 280

os.makedirs("data", exist_ok=True)

print(f"Reading {H_FILE} ...")
with open(H_FILE, "r") as f:
    content = f.read()

# Split into individual frame blocks by finding each array declaration
# Matches: const uint16_t gifframe_XX_...[] PROGMEM = { ... };
frame_blocks = re.findall(
    r'const\s+uint16_t\s+gifframe_\w+\[\]\s+PROGMEM\s*=\s*\{([^}]+)\}',
    content
)

print(f"Found {len(frame_blocks)} frames")

if len(frame_blocks) == 0:
    print("ERROR: No frames found — check animation.h path and format")
    exit(1)

expected_pixels = FRAME_W * FRAME_H

with open(OUT_FILE, "wb") as out:
    for i, block in enumerate(frame_blocks):
        # Extract all hex values like 0x1A2B
        values = re.findall(r'0x[0-9A-Fa-f]{4}', block)

        if len(values) != expected_pixels:
            print(f"  WARNING frame {i}: got {len(values)} pixels, expected {expected_pixels}")

        for v in values:
            out.write(struct.pack(">H", int(v, 16)))

        if i % 10 == 0 or i == len(frame_blocks) - 1:
            print(f"  Frame {i+1}/{len(frame_blocks)} done")

size_kb = os.path.getsize(OUT_FILE) / 1024
print(f"\nDone!")
print(f"Output : {OUT_FILE}")
print(f"Size   : {size_kb:.1f} KB  ({size_kb/1024:.2f} MB)")
print(f"\nSet in main.cpp:  #define TOTAL_FRAMES {len(frame_blocks)}")