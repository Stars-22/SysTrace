#!/usr/bin/env python3
"""
Generate src/embed_index.h from web/index.html.

Inlines web/style.css and web/app.js into the HTML before embedding,
so the final exe serves a single self-contained page.

Usage: python scripts/embed_index.py
"""

import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
INPUT_FILE = os.path.join(PROJECT_DIR, "web", "index.html")
CSS_FILE = os.path.join(PROJECT_DIR, "web", "style.css")
JS_FILE = os.path.join(PROJECT_DIR, "web", "app.js")
OUTPUT_FILE = os.path.join(PROJECT_DIR, "src", "embed_index.h")

HEADER = """// embed_index.h - Auto-generated from web/index.html by scripts/embed_index.py
// DO NOT EDIT MANUALLY - modify web/index.html, style.css, app.js instead and re-run the script

#pragma once

constexpr const char* INDEX_HTML = R"rawliteral("""

FOOTER = """)rawliteral";
"""


def main():
    with open(INPUT_FILE, "r", encoding="utf-8") as f:
        html = f.read()

    if os.path.exists(CSS_FILE):
        with open(CSS_FILE, "r", encoding="utf-8") as f:
            css = f.read()
        html = re.sub(
            r'<link\s+rel="stylesheet"\s+href="style\.css"\s*>',
            "<style>\n" + css + "\n</style>",
            html,
        )

    if os.path.exists(JS_FILE):
        with open(JS_FILE, "r", encoding="utf-8") as f:
            js = f.read()
        html = re.sub(
            r'<script\s+src="app\.js"\s*></script>',
            "<script>\n" + js + "\n</script>",
            html,
        )

    output = HEADER + html + FOOTER

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(output)

    print(f"Generated {OUTPUT_FILE} from {INPUT_FILE} ({len(html)} bytes)")


if __name__ == "__main__":
    main()
