# Firmware tools

Helper scripts used to build, publish and preview the PETALOT firmware.
All are plain Python 3 / Bash with no third-party packages.

## webui_preview.py — preview the web UI without flashing

The whole UI is a single C raw string inside `petalot/web_ui.h`. This script
extracts it and serves it over HTTP, answering the device API (`/get`, `/tele`,
`/set`, `/reset`, `/updatecheck`, `/update`) so you can open the UI in a
browser and iterate on it. `web_ui.h` is re-read on every page request, so
editing it and pressing **F5** is enough — no build, no flash.

```bash
Firmware/tools/webui_preview.py                        # mock, http://0.0.0.0:8080
Firmware/tools/webui_preview.py --device http://192.168.4.1   # proxy to a device
Firmware/tools/webui_preview.py --port 9000 --open
```

Two modes:

- **Mock** (default): answers the API with synthetic data (a slowly warming
  temperature, moving counters, editable settings). Good for layout, i18n and
  styling work with no hardware at all.
- **Proxy** (`--device URL`): serves the local HTML but forwards the API to a
  real device, so you preview your UI changes against live data. Use the device
  AP address (`http://192.168.4.1`) or its LAN IP.

Options:

| Option | Meaning |
| --- | --- |
| `--host` | bind address (default `0.0.0.0`, reachable from your phone) |
| `--port` | port (default `8080`) |
| `--device URL` | proxy the device API to this URL instead of mock data |
| `--open` | open the preview in the default browser |

Note: the online update check downloads `latest.json` and the `.bin` from
GitHub **in your browser**, so that part works in the preview too.

## check_web_ui.py — static UI checks

The UI lives in one big string; a stray apostrophe or unbalanced brace breaks
it silently. This checker catches that plus i18n key mismatches. `release.sh`
runs it before every build.

```bash
python3 Firmware/tools/check_web_ui.py [Firmware/petalot/web_ui.h]
```

## gen_latest_json.py — regenerate the update manifest

Writes `latest.json` (the file the browser reads for the online update) with
the version, file name and size of each PCB build.

```bash
python3 Firmware/tools/gen_latest_json.py <VERSION>
```

## release.sh — build and publish a release

Builds the three PCB flavours (1405 / 1501 / 1502), writes the versioned
binaries `petalot.<hw>-v<fw>.bin`, deletes the previous versioned builds,
regenerates `latest.json` and updates the file names in `Firmware/README.md`.

```bash
Firmware/tools/release.sh          # build with the VERSION already in petalot.ino
Firmware/tools/release.sh 1601     # set VERSION 1601 (= 1.6.1) and build

git add Firmware/petalot/petalot.ino Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/ Firmware/README.md
git commit -m "firmware: v1.6.1"
git push
```

Environment overrides: `ARDUINO_CLI`, `ARDUINO_CLI_CONFIG`, `FQBN`.
