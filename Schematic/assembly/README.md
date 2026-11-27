# PCB v1.5.3 — JLCPCB assembly files

Gerber: [`../Gerber_v1.5.3_2026-11-27.zip`](../Gerber_v1.5.3_2026-11-27.zip)

The PCB is 53 × 43 mm, two layers. The assembly is single-sided (top).

## Files

- `BOM_JLCPCB_v1.5.3_full.csv` — every soldered part, including the five 8-pin female headers.
- `CPL_JLCPCB_v1.5.3_full.csv` — pick-and-place for the full assembly.
- `BOM_JLCPCB_v1.5.3_no-headers.csv` — assembly without the five 8-pin female headers.
- `CPL_JLCPCB_v1.5.3_no-headers.csv` — pick-and-place without those headers.
- `source/` — original EasyEDA Pro exports.

Use the `no-headers` files to save a little: the Wemos and the other modules can supply
the 8-pin headers. Use the `full` files if you want JLCPCB to solder the female headers too.

## What JLCPCB solders

C1, C2, C3, CN4, CN5, CN6, H4, H5, H6, Q1, R1, R2, R3, R4, and — with the full files —
P6, P7, P8, P9 and P12.

These are **not** soldered and are bought separately as plug-in modules:

- DC-DC 5 V fixed module
- A4988 stepper driver
- Wemos D1 mini

The EasyEDA BOM export does not list the five 8-pin female headers, although they are
present in the CPL. The full BOM adds them manually with LCSC `C429957`
(JILN `22850108ANG1SYA01`, 1×8P, 2.54 mm, female, straight).

## JLCPCB order

1. Upload the gerber zip and choose 2 layers, 1.6 mm, 5 pieces.
2. Enable **PCB Assembly**, top side.
3. Upload the BOM and CPL pair you want: `full` or `no-headers`.
4. Check that the parts list contains only the 19 (or 14) soldered references above.
5. Do **not** add the DC-DC, A4988 or Wemos to the assembly BOM.
6. Review the placement preview: origin, Y direction, rotation and polarity of C1, C2,
   C3, Q1, CN4, CN5 and CN6.
7. Confirm that JLCPCB accepts the through-hole connectors. It does for these parts,
   but the quote page must show them before paying.

The CPL was generated from the EasyEDA pick-and-place export. JLCPCB sometimes rotates
through-hole parts differently from EasyEDA. The placement preview is the final check.
