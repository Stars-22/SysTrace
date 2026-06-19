#!/usr/bin/env python3
"""
Generate src/embed_logo.h from logo.svg.

Embeds the SVG content as a C++ string constant so the HTTP server
can serve it at /logo.svg without an external file.

Usage: python scripts/embed_logo.py
"""

import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SVG_FILE = os.path.join(PROJECT_DIR, "logo.svg")
OUTPUT_FILE = os.path.join(PROJECT_DIR, "src", "embed_logo.h")

HEADER = """// embed_logo.h - Auto-generated from logo.svg by scripts/embed_logo.py
// DO NOT EDIT MANUALLY - modify logo.svg instead and re-run the script

#pragma once

constexpr const char* LOGO_SVG = R"rawliteral("""

FOOTER = """)rawliteral";
"""


def main():
    with open(SVG_FILE, "r", encoding="utf-8") as f:
        svg = f.read()

    output = HEADER + svg + FOOTER

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(output)

    print(f"Generated {OUTPUT_FILE} from {SVG_FILE} ({len(svg)} bytes)")


if __name__ == "__main__":
    main()
