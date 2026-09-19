#!/usr/bin/env python3
"""Preview the PETALOT web UI in a browser without flashing the firmware.

The whole UI is a single C raw string literal inside web_ui.h. This tool
extracts it and serves it over HTTP, answering the device API (/get, /tele,
/set, /reset, /updatecheck, /update) either with synthetic data (mock) or by
forwarding to a real device (proxy).

Edit web_ui.h like usual and reload the browser with F5: the file is re-read
on every page request, so there is no build or flash step.

Usage:
    python3 webui_preview.py                         # mock on 0.0.0.0:8080
    python3 webui_preview.py --device http://192.168.4.1   # proxy to a device
    python3 webui_preview.py --port 9000 --open
"""

import argparse
import json
import re
import sys
import threading
import time
import urllib.error
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parent.parent          # Firmware/
SKETCH = ROOT / "petalot"
WEB_UI = SKETCH / "web_ui.h"
INO = SKETCH / "petalot.ino"

# Routes the UI calls on the device that we answer (mock) or forward (proxy).
API_ROUTES = ("/get", "/tele", "/set", "/reset", "/updatecheck", "/update")

START_MARK = 'R"rawliteral('
END_MARK = ')rawliteral"'


# ---------------------------------------------------------------------------
# Source parsing
# ---------------------------------------------------------------------------
def extract_html():
    """Return the UI HTML from web_ui.h, re-read every call."""
    text = WEB_UI.read_text(encoding="utf-8")
    if START_MARK not in text or END_MARK not in text:
        raise ValueError("web_ui.h: raw string literal markers not found")
    body = text[text.index(START_MARK) + len(START_MARK):text.index(END_MARK)]
    if "<html" not in body.lower():
        raise ValueError("web_ui.h: <html> block not found")
    return body


def read_macro(name, default):
    try:
        text = INO.read_text(encoding="utf-8")
    except OSError:
        return default
    m = re.search(r"^#define\s+%s\s+([0-9]+)" % re.escape(name), text, re.M)
    return int(m.group(1)) if m else default


def human_version(v):
    return "%d.%d.%d" % (v // 1000, (v % 1000) // 100, v % 100)


# ---------------------------------------------------------------------------
# Mock device state
# ---------------------------------------------------------------------------
class MockState:
    def __init__(self):
        self.version = human_version(read_macro("VERSION", 1600))
        self.pcb = read_macro("PCB", 1502)
        self.t0 = time.time()
        self.conf = {
            "To": 65, "Vo": 6, "Fenable": True,
            "ssid": "HomeWifi", "password": "secret",
            "LocalIP": "", "Subnet": "", "Gateway": "",
            "Gate": 2, "MaxGate": 30, "HYS": 1.0, "RAMP": 5.0, "HOLD": 40,
            "Kp": 12.0, "Ki": 0.5, "Kd": 30.0, "ControlMode": 0,
            "TOffset": 0, "Stopdelay": 5, "Maxtime": 0, "NoFilamentTime": 30,
            "UseDisplay": True, "StartOnPower": False, "MotorOnTo": False,
            "pcbVer": self.pcb, "version": self.version,
            "minT": 0, "maxT": 250, "minV": 1, "maxV": 30,
        }
        self.status = 0          # 0 stopped, 1 running (no motor), 2 running

    def tele(self):
        dt = time.time() - self.t0
        running = self.status != 0
        temp = self.conf["To"] + (2.0 * ((dt % 4) - 2)) if running else 22.0
        out = 60 if running and temp < self.conf["To"] else (12 if running else 0)
        return {
            "status": self.status,
            "T": round(temp, 1),
            "AR": 512,
            "To": self.conf["To"],
            "Vo": self.conf["Vo"],
            "F": True,
            "Fenable": self.conf["Fenable"],
            "Ft": round(dt / 3.0, 1),
            "Fs": round(dt / 3.0, 1),
            "Tt": int(dt),
            "Ts": int(dt),
            "LastStopReason": "",
            "Output": "%d%%" % out,
        }

    def apply(self, params):
        for k, vals in params.items():
            v = vals[-1]
            if k in ("To", "Vo"):
                # +/- relative changes, like the device's single-step buttons
                try:
                    delta = int(v)
                except ValueError:
                    continue
                if k == "To":
                    self.conf["To"] = max(0, min(250, self.conf["To"] + delta))
                else:
                    self.conf["Vo"] = max(1, min(30, self.conf["Vo"] + delta))
            elif k == "status":
                try:
                    self.status = int(v)
                except ValueError:
                    pass
            elif k == "Fenable":
                self.conf["Fenable"] = (v == "true")
            elif k in ("StartOnPower", "MotorOnTo", "UseDisplay"):
                self.conf[k] = (v == "true")
            elif k in ("ssid", "password", "LocalIP", "Subnet", "Gateway"):
                self.conf[k] = v
            elif k in ("ControlMode", "Gate", "MaxGate", "TOffset",
                       "Stopdelay", "Maxtime", "NoFilamentTime"):
                try:
                    self.conf[k] = int(float(v))
                except ValueError:
                    pass
            elif k in ("HYS", "RAMP", "HOLD", "Kp", "Ki", "Kd"):
                try:
                    self.conf[k] = float(v)
                except ValueError:
                    pass
            # 'reboot' and unknown keys are ignored in mock mode


# ---------------------------------------------------------------------------
# HTTP handler
# ---------------------------------------------------------------------------
class Handler(BaseHTTPRequestHandler):
    server_version = "PetalotPreview"

    # injected by the server
    args = None
    mock = None
    proxy = None

    def log_message(self, fmt, *a):
        sys.stderr.write("  %s\n" % (fmt % a))

    # -- helpers ------------------------------------------------------------
    def _send(self, code, body, ctype="application/json"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body)
        data = body.encode("utf-8") if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        try:
            self.wfile.write(data)
        except BrokenPipeError:
            pass

    def _proxy(self, method):
        url = self.proxy.rstrip("/") + self.path
        length = int(self.headers.get("Content-Length") or 0)
        payload = self.rfile.read(length) if length else None
        req = urllib.request.Request(url, data=payload, method=method)
        ctype = self.headers.get("Content-Type")
        if ctype:
            req.add_header("Content-Type", ctype)
        try:
            with urllib.request.urlopen(req, timeout=20) as resp:
                self._send(resp.status, resp.read(),
                           resp.headers.get("Content-Type", "application/json"))
        except urllib.error.HTTPError as e:
            self._send(e.code, e.read(),
                       e.headers.get("Content-Type", "text/plain"))
        except Exception as e:  # device unreachable
            self._send(502, "proxy error: %s" % e, "text/plain")

    # -- routing ------------------------------------------------------------
    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/" or path == "/index.html":
            try:
                self._send(200, extract_html(), "text/html; charset=utf-8")
            except Exception as e:
                self._send(500, "web_ui.h error: %s" % e, "text/plain")
            return
        if self.proxy:
            if path in API_ROUTES:
                self._proxy("GET")
                return
        if path == "/get":
            self._send(200, self.mock.conf)
        elif path == "/tele":
            self._send(200, self.mock.tele())
        elif path == "/set":
            self.mock.apply(parse_qs(urlparse(self.path).query, keep_blank_values=True))
            self._send(200, self.mock.tele())
        elif path == "/reset":
            self._send(200, "OK", "text/plain")
        elif path == "/updateinfo":
            self._send(404, "not found", "text/plain")
        else:
            self._send(404, "not found", "text/plain")

    def do_POST(self):
        path = urlparse(self.path).path
        if self.proxy and path in API_ROUTES:
            self._proxy("POST")
            return
        # drain the multipart body (manual update posts a whole .bin)
        length = int(self.headers.get("Content-Length") or 0)
        if length:
            self.rfile.read(length)
        if path == "/updatecheck":
            self._send(200, "OK", "text/plain")
        elif path == "/update":
            self._send(200, "Update success, rebooting...", "text/plain")
        else:
            self._send(404, "not found", "text/plain")


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    p = argparse.ArgumentParser(
        prog="webui_preview.py",
        description="Preview the PETALOT web UI without flashing the firmware.",
        epilog=(
            "The UI HTML is extracted from Firmware/petalot/web_ui.h and re-read "
            "on every page load, so edit the file and press F5 to see changes.\n\n"
            "Mock mode (default) answers the device API with synthetic data. "
            "Proxy mode (--device URL) forwards /get, /tele, /set, /reset, "
            "/updatecheck and /update to a real device while still serving the "
            "local HTML, so you can iterate on the UI with real data."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("--host", default="0.0.0.0",
                   help="bind address (default: 0.0.0.0, reachable from phone)")
    p.add_argument("--port", type=int, default=8080,
                   help="port (default: 8080)")
    p.add_argument("--device", default=None, metavar="URL",
                   help="proxy the device API to this URL, e.g. http://192.168.4.1")
    p.add_argument("--open", action="store_true",
                   help="open the preview in the default browser")
    args = p.parse_args()

    if not WEB_UI.is_file():
        print("error: %s not found" % WEB_UI, file=sys.stderr)
        return 1

    Handler.args = args
    Handler.proxy = args.device
    Handler.mock = MockState()

    httpd = ThreadingHTTPServer((args.host, args.port), Handler)
    url = "http://%s:%d/" % ("localhost" if args.host in ("0.0.0.0", "::")
                             else args.host, args.port)
    mode = "proxy -> %s" % args.device if args.device else "mock"
    print("PETALOT web UI preview")
    print("  source: %s" % WEB_UI)
    print("  mode:   %s" % mode)
    print("  url:    %s" % url)
    print("  edit web_ui.h and press F5 to reload. Ctrl+C to stop.")
    if args.open:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        httpd.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
