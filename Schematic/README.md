# PETALOT electronics

## PCB v1.5.3 — recommended

Gerber: [`Gerber_v1.5.3_2026-11-27.zip`](Gerber_v1.5.3_2026-11-27.zip)

Order it assembled at JLCPCB. All parts are soldered except the DC-DC, A4988 and
Wemos, which plug into the board. The assembly files and order notes are in
[`assembly/`](assembly/README.md).

The PCB v1.5.2 and v1.5.3 share the same pin map. The difference is the gearbox
ratio used by the firmware: 1:46 on v1.5.2 and 1:50 on v1.5.3.

## PCB v1.1 — legacy DIY

Gerber: [`Gerber_v1.1_2023-01-02.zip`](Gerber_v1.1_2023-01-02.zip)

This is the old bare board. It needs the resistors, capacitors, MOSFET, headers and
screw terminals soldered by hand. It remains available for anyone who wants to build
the old version, but it is **not recommended** for new builds.
