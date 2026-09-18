<img width="300" alt="image" src="https://github.com/user-attachments/assets/5ee03bce-7dc0-4dd2-b8da-baeee425e0f6" />

# PETALOT Firmware

There is **one firmware**, built separately for each PCB version. The three
builds are functionally identical and share the same web UI; they only differ
in the microcontroller pin mapping (defined in `pins.hpp`). Pick the `.bin`
that matches the PCB you have installed.

## Which firmware do I need?

| PCB version | Firmware file | Availability |
| --- | --- | --- |
| **v1.4.5** | [`petalot.1.4.5-v1.6.0.bin`](Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.4.5-v1.6.0.bin) | **DIY** — the only board released publicly. |
| **v1.5.1** | [`petalot.1.5.1-v1.6.0.bin`](Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.5.1-v1.6.0.bin) | **Commercial**. |
| **v1.5.2** | [`petalot.1.5.2-v1.6.0.bin`](Firmware/petalot/build/esp8266.esp8266.d1_mini_clone/petalot.1.5.2-v1.6.0.bin) | **Commercial** (current). |

- If you built your own PETALOT from the public files, your PCB is the **v1.4.5**.
- Versions **v1.5.1** and **v1.5.2** are only fitted on commercially sold machines.

> Do not flash a firmware that does not match your PCB.

## Identify your PCB

| v1.4.5 (DIY) | v1.5.1 | v1.5.2 |
| --- | --- | --- |
| ![PCB v1.4.5](PCB_v1.4.5.jpeg) | ![PCB v1.5.1](PCB_v1.5.1.jpeg) | ![PCB v1.5.2](PCB_v1.5.2.jpeg) |

## First-time flash

1. Connect the Wemos to your PC.
2. Go to https://web.esphome.io/
3. Click **Connect**, select **USB Serial** port and **Connect**.
4. Click **Install**, then **Choose File**, select the `.bin` for your PCB and click **Install**.
5. Connect your phone to the Wi-Fi "PETALOT-XXXXXX" and browse to 192.168.4.1.
6. (Optional) Enter your local Wi-Fi SSID, password, IP address and subnet.

## Updating an existing device

You do not need a cable: update over the air.

### Online update (needs Internet in the browser)

Open the web UI and go to **Settings → Advanced → Online update**, then click
**Check for updates**. If a newer firmware for your PCB is available, click
**Install update**.

The manifest (`latest.json`) and the firmware image are downloaded from this
repository by **your browser**, not by the device, and then uploaded to the
device over the same path as a manual update. This means only the computer or
phone running the browser needs Internet access; the device itself can be in AP
mode or on a network without Internet.

### Manual update (works offline)

1. Browse to `http://<device-ip>/update` (the built-in OTA page). This is the
   update method available on **every** firmware version — older builds only
   ship this one.
2. Select the `.bin` matching your PCB (`petalot.1.4.5-v1.6.0.bin`,
   `petalot.1.5.1-v1.6.0.bin` or `petalot.1.5.2-v1.6.0.bin`) and click **Update**.

If your device already runs a recent build, you can also use **Settings →
Advanced → Firmware Update** in the web UI — same result.

## Building from source

1. Open `Firmware/petalot/petalot.ino` in the Arduino IDE.
2. Set `PCB` to the hardware you are compiling for:

   | `PCB` | Hardware |
   | --- | --- |
   | `1405` | v1.4.5 |
   | `1501` | v1.5.1 |
   | `1502` | v1.5.2 |

3. Set `VERSION` to the firmware version you are publishing (e.g. `1600`).
   `VERSION` is independent of the PCB revision and is what the online update
   compares against `latest.json`.
4. Select **Tools > Board > esp8266 > LOLIN(WEMOS) D1 mini (clone)**.
5. Compile and upload.

The pin mapping for every hardware revision lives in `Firmware/petalot/pins.hpp`.
See [`INSTALL_LIBRARIES.md`](Firmware/petalot/INSTALL_LIBRARIES.md) for the
required libraries and board setup.

### Publishing an online update

1. Build the three firmwares (one per `PCB`) with the new `VERSION`.
2. Regenerate the manifest:

   ```
   python3 Firmware/tools/gen_latest_json.py <VERSION>
   ```

   This writes `latest.json` next to the `.bin` files, with the version, file
   name and size for every PCB.
3. Commit and push the three `.bin` files and `latest.json` to the repository
   (the online update reads them from `master`).
