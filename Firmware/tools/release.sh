#!/usr/bin/env bash
#
# Build the three PETALOT PCB flavours and regenerate latest.json for the
# GitHub online update.
#
# Usage:
#   Firmware/tools/release.sh          # use the VERSION already in petalot.ino
#   Firmware/tools/release.sh 1601     # set VERSION to 1601 (= 1.6.1) and build
#
# It compiles PCB 1405 / 1501 / 1502 (same source, only `#define PCB` changes),
# writes the binaries to
#   petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.4.5-v<fw>.bin  (PCB 1405)
#   petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.5.1-v<fw>.bin  (PCB 1501)
#   petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.5.2-v<fw>.bin  (PCB 1502)
# (only the latest build per PCB is kept; the legacy petalot.v1.2.bin and
# petalot.1.4.4.bin are left untouched) and refreshes latest.json with the new
# version and sizes.
#
# On success petalot.ino keeps the new VERSION and the original PCB is
# restored, so commit everything together:
#   git add Firmware/petalot/petalot.ino Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/
#   git commit -m "firmware: v<major>.<minor>.<patch>"
#   git push
#
# Environment overrides:
#   ARDUINO_CLI         path to arduino-cli (auto-detected otherwise)
#   ARDUINO_CLI_CONFIG  path to arduino-cli.yaml (defaults to the IDE one)
#   FQBN                fully qualified board name (defaults to the IDE config)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$(cd "$SCRIPT_DIR/../petalot" && pwd)"
INO="$SKETCH_DIR/petalot.ino"
BUILD_DIR="$SKETCH_DIR/build/esp8266.esp8266.d1_mini_clone"

PCBS=(1405 1501 1502)
# Hardware revision as shown to users (PCB 1405 -> 1.4.5, etc.)
HWS=(1.4.5 1.5.1 1.5.2)

FQBN="${FQBN:-esp8266:esp8266:d1_mini_clone:baud=921600,xtal=80,eesz=4M2M,FlashMode=dout,FlashFreq=40,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,wipe=none,ssl=all,mmu=3232,non32xfer=fast}"

# --- locate arduino-cli -----------------------------------------------------
find_cli() {
  if [ -n "${ARDUINO_CLI:-}" ]; then printf '%s\n' "$ARDUINO_CLI"; return; fi
  if command -v arduino-cli >/dev/null 2>&1; then command -v arduino-cli; return; fi
  find /var/lib/flatpak/app/cc.arduino.IDE2 -type f -name arduino-cli 2>/dev/null | head -1
}
CLI="$(find_cli || true)"
if [ -z "$CLI" ] || [ ! -x "$CLI" ]; then
  echo "error: arduino-cli not found; set ARDUINO_CLI=/path/to/arduino-cli" >&2
  exit 1
fi

CLI_ARGS=()
for cfg in "${ARDUINO_CLI_CONFIG:-}" "$HOME/.arduinoIDE/arduino-cli.yaml" "$HOME/.arduino15/arduino-cli.yaml"; do
  if [ -n "$cfg" ] && [ -f "$cfg" ]; then CLI_ARGS=(--config-file "$cfg"); break; fi
done

# --- resolve version --------------------------------------------------------
orig_pcb="$(sed -n 's/^#define PCB \([0-9][0-9]*\).*/\1/p' "$INO")"
[ -n "$orig_pcb" ] || { echo "error: could not read PCB from $INO" >&2; exit 1; }

version="$(sed -n 's/^#define VERSION \([0-9][0-9]*\).*/\1/p' "$INO")"
if [ "$#" -ge 1 ]; then
  case "$1" in
    ''|*[!0-9]*) echo "error: version must be a positive integer" >&2; exit 1 ;;
  esac
  version="$1"
  sed -i "s/^#define VERSION [0-9][0-9]*/#define VERSION $version/" "$INO"
fi
[ -n "$version" ] || { echo "error: could not read VERSION from $INO" >&2; exit 1; }

major=$((version / 1000))
minor=$(((version % 1000) / 100))
patch=$((version % 100))
human="$major.$minor.$patch"

# Keep only the latest build for the active PCBs (the legacy petalot.v1.2.bin
# and petalot.1.4.4.bin are left untouched).
mkdir -p "$BUILD_DIR"
rm -f "$BUILD_DIR"/petalot.1.4.5-v*.bin "$BUILD_DIR"/petalot.1.5.1-v*.bin "$BUILD_DIR"/petalot.1.5.2-v*.bin

# --- sanity-check the web UI before spending a full build -------------------
if [ -f "$SCRIPT_DIR/check_web_ui.py" ]; then
  echo "==> checking web_ui.h"
  if ! python3 "$SCRIPT_DIR/check_web_ui.py" "$SKETCH_DIR/web_ui.h"; then
    echo "error: web_ui.h failed static checks (see above); aborting" >&2
    exit 1
  fi
fi

# --- work dir + restore on exit --------------------------------------------
WORK="$(mktemp -d)"
OUT="$WORK/out"
BUILD_PATH="$WORK/build"
LOG="$WORK/build.log"
mkdir -p "$OUT" "$BUILD_PATH" "$BUILD_DIR"

BACKUP="$(mktemp)"
cp "$INO" "$BACKUP"
done_flag=0
restore() {
  if [ "$done_flag" -eq 1 ]; then
    # keep the new VERSION, put the original PCB back
    sed -i "s/^#define PCB [0-9][0-9]*/#define PCB $orig_pcb/" "$INO"
  else
    cp "$BACKUP" "$INO"
  fi
  rm -f "$BACKUP"
  rm -rf "$WORK"
}
trap restore EXIT

# --- build each PCB ---------------------------------------------------------
for i in "${!PCBS[@]}"; do
  pcb="${PCBS[$i]}"
  hw="${HWS[$i]}"
  file="petalot.${hw}-v${human}.bin"
  echo "==> PCB $pcb -> $file"
  sed -i "s/^#define PCB [0-9][0-9]*/#define PCB $pcb/" "$INO"
  if ! "$CLI" "${CLI_ARGS[@]}" compile --fqbn "$FQBN" \
        --output-dir "$OUT" --build-path "$BUILD_PATH" "$SKETCH_DIR" > "$LOG" 2>&1; then
    echo "error: compile failed for PCB $pcb" >&2
    tail -40 "$LOG" >&2
    exit 1
  fi
  if ! grep -aq "PETALOT-PCB-$pcb" "$OUT/petalot.ino.bin"; then
    echo "error: marker PETALOT-PCB-$pcb missing in built binary" >&2
    exit 1
  fi
  cp "$OUT/petalot.ino.bin" "$BUILD_DIR/$file"
done

# --- regenerate manifest ----------------------------------------------------
python3 "$SCRIPT_DIR/gen_latest_json.py" "$version"
done_flag=1

echo
echo "Built firmware version $human (VERSION $version)."
echo "Next:"
echo "  git add Firmware/petalot/petalot.ino Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/"
echo "  git commit -m \"firmware: v$human\""
echo "  git push"
