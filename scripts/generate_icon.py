#!/usr/bin/env python3
"""
Generate logo.ico from logo.svg using resvg + PIL.

Usage: python scripts/generate_icon.py
"""

import os
import io

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SVG_FILE = os.path.join(PROJECT_DIR, "logo.svg")
ICO_FILE = os.path.join(PROJECT_DIR, "logo.ico")

ICO_SIZES = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]


def main():
    try:
        import resvg
        from resvg import usvg
    except ImportError:
        print("Error: resvg package not installed. Run: pip install resvg")
        return

    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow not installed. Run: pip install Pillow")
        return

    with open(SVG_FILE, "r", encoding="utf-8") as f:
        svg_data = f.read()

    opts = usvg.Options.default()
    try:
        opts.load_system_fonts()
    except Exception:
        pass
    tree = usvg.Tree.from_str(svg_data, opts)

    png_bytes = resvg.render(tree, transform=(1.0, 0.0, 0.0, 1.0, 0.0, 0.0))
    img = Image.open(io.BytesIO(png_bytes)).convert("RGBA")

    img.save(ICO_FILE, format="ICO", sizes=ICO_SIZES)
    print(f"Generated {ICO_FILE} ({os.path.getsize(ICO_FILE)} bytes)")


if __name__ == "__main__":
    main()
