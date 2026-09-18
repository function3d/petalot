#!/usr/bin/env python3
"""Static sanity checks for the PETALOT web UI (web_ui.h).

The whole UI lives inside a single C raw string literal with an inline
<script>. A single unescaped apostrophe inside a single-quoted JS string, or a
stray brace, silently breaks the entire UI (no styles, no buttons). This checker
catches those before flashing. It is not a full JS parser, only a practical
guard for this codebase.

Checks:
  * the raw literal / <script> block is present and balanced
  * every single/double quote, template literal and comment is closed
  * no JS string is closed by an apostrophe followed by an identifier
    (the classic "l'accesso" bug)
  * { } ( ) [ ] are balanced outside strings/comments
  * i18n keys are the same set in every language dictionary

Exits non-zero and prints the offending line on failure.

Usage:
    python3 check_web_ui.py [path/to/web_ui.h]
"""

import re
import sys
from pathlib import Path

DEFAULT = Path(__file__).resolve().parent.parent / "petalot" / "web_ui.h"


def extract_js(html):
    marker_start = 'R"rawliteral('
    marker_end = ')rawliteral"'
    if marker_start not in html or marker_end not in html:
        raise ValueError("web_ui.h: raw string literal markers not found")
    body = html[html.index(marker_start) + len(marker_start):html.index(marker_end)]
    if "<script>" not in body or "</script>" not in body:
        raise ValueError("web_ui.h: <script> block not found")
    return body[body.index("<script>") + len("<script>"):body.index("</script>")], body


# After '('/'=' / ',' / ':' / '[' / '!' / '&' / '|' / '?' / ';' / '{' / '}'
# a '/' starts a regex literal rather than a division.
REGEX_PRECEDERS = set("(,=:[!&|?;{}") | {"\n"}


def scan(js):
    """Return list of (line, message)."""
    errors = []
    i, n = 0, len(js)
    line = 1
    depth = {"{": 0, "(": 0, "[": 0}
    close = {"}": "{", ")": "(", "]": "["}
    state = "code"
    last_sig = ""  # last significant char in code, for regex detection

    def err(msg):
        errors.append((line, msg))

    while i < n:
        c = js[i]
        if c == "\n":
            line += 1

        if state == "code":
            if js[i:i + 2] == "//":
                state = "lc"
                i += 2
                continue
            if js[i:i + 2] == "/*":
                state = "bc"
                i += 2
                continue
            if c == "/" and last_sig in REGEX_PRECEDERS:
                state = "regex"
                i += 1
                continue
            if c == '"':
                state = "dq"
            elif c == "'":
                state = "sq"
            elif c == "`":
                state = "tick"
            elif c in depth:
                depth[c] += 1
                last_sig = c
            elif c in close:
                depth[close[c]] -= 1
                if depth[close[c]] < 0:
                    err(f"unbalanced '{c}'")
                last_sig = c
            elif not c.isspace():
                last_sig = c
        elif state == "lc":
            if c == "\n":
                state = "code"
        elif state == "bc":
            if js[i:i + 2] == "*/":
                state = "code"
                i += 2
                continue
        elif state == "dq":
            if c == "\\":
                i += 2
                continue
            if c == '"':
                state = "code"
                last_sig = '"'
        elif state == "tick":
            if c == "\\":
                i += 2
                continue
            if c == "`":
                state = "code"
                last_sig = "`"
        elif state == "sq":
            if c == "\\":
                i += 2
                continue
            if c == "'":
                nxt = js[i + 1] if i + 1 < n else ""
                # 'abc'def or 'l'accesso' => closing quote followed by identifier
                if nxt.isalpha() or nxt == "_" or nxt == "$":
                    err("apostrophe inside a single-quoted string "
                        "(use double quotes and escape)")
                state = "code"
                last_sig = "'"
        elif state == "regex":
            if c == "\\":
                i += 2
                continue
            if c == "/":
                state = "code"
                last_sig = "/"
        i += 1

    if state != "code":
        err(f"unterminated {state} (string/comment/regex not closed)")
    for opener, d in depth.items():
        if d != 0:
            err(f"unbalanced '{opener}' (depth {d})")
    return errors


# Matches  'key': {  ... }  per language block.
LANG_RE = re.compile(r"\n\s*([a-zA-Z]{2}(?:-[A-Za-z]{2})?)\s*:\s*\{")


def i18n_parity(js):
    """Return list of (line, message) for i18n key mismatches."""
    matches = list(LANG_RE.finditer(js))
    if len(matches) < 2:
        return []
    tables = []
    for m in matches:
        start = m.end()
        depth = 1
        j = start
        while j < len(js) and depth:
            ch = js[j]
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
            j += 1
        block = js[start:j - 1]
        keys = set(re.findall(r"['\"]([A-Za-z0-9_.]+)['\"]\s*:", block))
        tables.append((m.group(1), js[:m.start()].count("\n") + 1, keys))

    ref_lang, ref_line, ref = tables[0]
    errors = []
    for lang, ln, keys in tables[1:]:
        missing = ref - keys
        extra = keys - ref
        if missing or extra:
            bits = []
            if missing:
                bits.append("missing " + ", ".join(sorted(missing)))
            if extra:
                bits.append("extra " + ", ".join(sorted(extra)))
            errors.append((ln, f"i18n '{lang}' differs from '{ref_lang}': "
                               + "; ".join(bits)))
        if ref_line:  # silence linters
            pass
    return errors


def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    html = path.read_text(encoding="utf-8")
    js, body = extract_js(html)

    errors = scan(js)
    errors += i18n_parity(js)

    if errors:
        print(f"{path}: FAIL ({len(errors)} problem(s))")
        for ln, msg in errors[:40]:
            print(f"  line {ln}: {msg}")
        return 1

    print(f"{path}: OK ({len(js)} chars of JS)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
