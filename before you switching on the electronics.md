# Before switching on the electronics for the first time

## PCB v1.5.3 assembled by JLCPCB

The PCB v1.5.3 uses a fixed 5 V DC-DC module. It does **not** need calibration:
there is no potentiometer to adjust and no 5 V jumper to cut or bridge.

Check that the DC-DC, A4988 and Wemos are fully seated in their headers before
powering the board.

## Power supply and hotend voltage

Use the same voltage for the power supply and the hotend:

- 12 V power supply + 12 V hotend.
- 24 V power supply + 24 V hotend.

Never mix them. A 24 V supply with a 12 V hotend will destroy the hotend.

Check the polarity of the power supply before connecting it. Reversing the
supply can permanently damage the board.

## Adjust the reference voltage of the stepper driver

You have to adjust it to supply 1 A to the motor. This way the motor will hardly
heat up and it will have enough power.

- Turn off the electronics.
- Disconnect the motor.
- Turn on the electronics.
- Stop from the web interface so that the hotend is not heating up.
- Measure the voltage between the small potentiometer on the driver and ground.
- Turn the potentiometer until you measure about 0.4 V (0.4 × 2.5 = 1 A).
- Once adjusted, turn off the electronics.
- Connect the motor.
- Turn on the electronics again.

Never connect or disconnect the motor while the electronics are on.

## Legacy PCB v1.1 with adjustable DC-DC

This section applies only to the old bare PCB v1.1 and its adjustable DC-DC
module. The new assembled PCB v1.5.3 does not need it.

### Set the DC-DC output to 5 V

Option 1:

- Put 12 V between IN and GND.
- With a multimeter, measure VO and GND.
- Turn the potentiometer until you measure 5 V.

Option 2:

- Cut the jumper on ADJ.
- Put a drop of solder on the 5 V jumper.

After setting the output, follow the A4988 adjustment above.
