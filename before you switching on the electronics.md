# Before switching on the electronics for the first time
## Set to 5V output on D3 DC-DC step down
### Option 1
- You put 12v between IN and GND
- With a multimeter you measure VO and GND
- Turn the potentiometer until you measure 5V
### Option 2
- Cut the jumper on ADJ
- Put a drop of tin on the 5V jumper


# Adjust the reference voltage of the stepper drive
You have to adjust it to supply 1A to the motor, this way the motor will hardly heat up and it will have enough power.
- Turn off the electronics
- Disconnect the motor
- Turn on the electronics
- Stop from the web interface so that the hotend is not heating up
- Measure the voltage between the small potentiometer on the driver and ground
- Turn the potentiometer so that the value is about 0.4V
- Once adjusted, turn off the electronics
- Connect the motor
- Turn on the electronics again

Never connect/disconnect the motor with the electronic  on
