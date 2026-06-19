#!/usr/bin/env python3
"""
Generate logo.ico from logo.svg using resvg-py + PIL (BMP/DIB via Pillow's BMP writer).

Usage: python scripts/generate_icon.py
"""

import os
import io
import struct

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SVG_FILE = os.path.join(PROJECT_DIR, "logo.svg")
ICO_FILE = os.path.join(PROJECT_DIR, "logo.ico")

ICO_SIZES = [(256, 256)]

BMP_HEADER_SIZE = 14


def main():
    try:
        import resvg_py
    except ImportError:
        print("Error: resvg_py package not installed. Run: pip install resvg-py")
        return

    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow not installed. Run: pip install Pillow")
        return

    with open(SVG_FILE, "r", encoding="utf-8") as f:
        svg_data = f.read()

    bmp_entries = []
    for w, h in ICO_SIZES:
        png_bytes = resvg_py.svg_to_bytes(svg_data, width=w, height=h)
        img = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
        bmp_data = _make_bmp_dib(img, w, h)
        bmp_entries.append((w, h, bmp_data))

    _write_ico(ICO_FILE, bmp_entries)
    print(f"Generated {ICO_FILE} ({os.path.getsize(ICO_FILE)} bytes)")


def _make_bmp_dib(img, w, h):
    # Use Pillow's BMP writer to get a properly encoded DIB
    buf = io.BytesIO()
    img.save(buf, format="BMP")
    bmp_data = buf.getvalue()

    # Strip 14-byte BMP file header to keep only DIB
    dib = bmp_data[BMP_HEADER_SIZE:]

    # Patch biHeight: BMP files use h, but ICO format requires 2*h
    # (combined height of XOR mask + AND mask)
    dib = struct.pack("<I", 40) + dib[4:8] + struct.pack("<i", h * 2) + dib[12:]

    # Extract XOR mask from DIB (after 40-byte BITMAPINFOHEADER)
    xor_stride = ((w * 32 + 31) // 32) * 4
    xor_size = xor_stride * h
    xor_data = dib[40:40 + xor_size]

    # AND mask: 1-bit per pixel, all zeros (transparency via alpha channel)
    and_row_bytes = ((w + 31) // 32) * 4
    and_mask = b"\x00" * (and_row_bytes * h)

    # DIB + AND mask in correct ICO order
    return dib[:40 + xor_size] + and_mask


def _write_ico(path, bmp_entries):
    image_count = len(bmp_entries)
    header = struct.pack("<HHH", 0, 1, image_count)

    dir_offset = 6 + image_count * 16
    entries = []
    for w, h, data in bmp_entries:
        disp_w = w if w < 256 else 0
        disp_h = h if h < 256 else 0
        size = len(data)
        entries.append(struct.pack("<BBBBHHII",
            disp_w, disp_h, 0, 0, 1, 32, size, dir_offset))
        dir_offset += size

    with open(path, "wb") as f:
        f.write(header)
        for entry in entries:
            f.write(entry)
        for _, _, data in bmp_entries:
            f.write(data)


if __name__ == "__main__":
    main()
