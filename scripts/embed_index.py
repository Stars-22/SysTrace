#!/usr/bin/env python3
"""
Generate src/embed_index.h from web/index.html.

Usage: python scripts/embed_index.py
"""

import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
INPUT_FILE = os.path.join(PROJECT_DIR, "web", "index.html")
OUTPUT_FILE = os.path.join(PROJECT_DIR, "src", "embed_index.h")

HEADER = """// embed_index.h - Auto-generated from web/index.html by scripts/embed_index.py
// DO NOT EDIT MANUALLY - modify web/index.html instead and re-run the script

#pragma once

constexpr const char* INDEX_HTML = R"rawliteral("""

FOOTER = """)rawliteral";
"""


def main():
    with open(INPUT_FILE, "r", encoding="utf-8") as f:
        html_content = f.read()

    output = HEADER + html_content + FOOTER

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(output)

    print(f"Generated {OUTPUT_FILE} from {INPUT_FILE} ({len(html_content)} bytes)")


if __name__ == "__main__":
    main()