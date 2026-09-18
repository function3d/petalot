#!/usr/bin/env python3
"""Regenerate latest.json for the PETALOT online update.

The device fetches latest.json from the GitHub raw build folder, picks the
entry matching its PCB revision (1405 / 1501 / 1502), and compares the
firmware version with its own `VERSION`.

Build each firmware by setting `PCB` to the matching revision and `VERSION`
to the new firmware version, then run this script with that same version.

Usage:
    python3 gen_latest_json.py <firmware_version> [build_dir]

Example:
    python3 gen_latest_json.py 1600
"""

import json
import os
import sys

# PCB revision -> hardware revision shown to users. The firmware file is named
# "petalot.<hw>-v<fw>.bin", e.g. petalot.1.5.2-v1.6.0.bin.
BOARDS = {
    "1405": "1.4.5",
    "1501": "1.5.1",
    "1502": "1.5.2",
}

DEFAULT_DIR = os.path.abspath(
    os.path.join(
        os.path.dirname(__file__),
        "..", "petalot", "build", "esp8266.esp8266.d1_mini_clone",
    )
)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    version = int(sys.argv[1])
    out_dir = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_DIR

    major = version // 1000
    minor = (version % 1000) // 100
    patch = version % 100
    fw = "%d.%d.%d" % (major, minor, patch)

    manifest = {}
    for pcb, hw in BOARDS.items():
        filename = "petalot.%s-v%s.bin" % (hw, fw)
        entry = {"version": version, "file": filename}
        path = os.path.join(out_dir, filename)
        if os.path.exists(path):
            entry["size"] = os.path.getsize(path)
        else:
            print("warning: %s not found, size omitted" % path)
        manifest[pcb] = entry

    out = os.path.join(out_dir, "latest.json")
    with open(out, "w") as handle:
        json.dump(manifest, handle, indent=2)
        handle.write("\n")
    print("wrote " + out)
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
