# Firmware

[Back to the project overview](../README.md)

[MongolToriMega.ino](MongolToriMega.ino) contains all configuration, control functions, and compile-time checks. No companion headers or third-party libraries are needed.

## Setup

1. Open the sketch in Arduino IDE. If prompted, allow it to place the file in a folder named `MongolToriMega`.
2. Select **Arduino Mega or Mega 2560 / ATmega2560**. Serial status uses **115200 baud**.
3. Check the configuration at the top of the sketch: receiver functions, motor groups, polarity, and pulse ranges.
4. After checking the wiring, change `ROVER_CONFIGURATION_CONFIRMED` from `0` to `1`. The supplied default keeps every PWM output disabled.
5. Compile and upload with motor power disconnected, then follow the [build checks](../docs/BUILD_NOTES.md).

## Pins from the schematic

Receiver CH1–CH6 connect to D2–D7. The sketch uses CH1/D2 for steering, CH2/D3 for throttle, and CH5/D6 for a two-position arm switch. Confirm these assignments in the transmitter.

| Driver location | Output | PWM | Direction | Default group |
| --- | --- | ---: | ---: | --- |
| Upper right | M1 | 10 | 15 | Left |
| Upper right | M2 | 11 | 14 | Right |
| Lower right | M1 | 9 | 17 | Left |
| Lower right | M2 | 8 | 16 | Right |
| Left | M1 | 12 | 19 | Left |
| Left | M2 | 13 | 18 | Right |

Pins follow the [schematic](../hardware/Sch-MT.jpg). M1=left/M2=right is an assumed wheel grouping; change each motor's `left` and `invert` values to match the rover. Drivers must use compatible independent PWM/direction control.

## Operation

With wheels clear, switch the arm channel LOW, center both sticks, then switch HIGH and hold neutral for one second. Returning the arm switch LOW stops outputs on the next arm-input read.

The initial output limit is 100/255. Speed changes gradually and passes through zero before reversing. Missing or invalid required pulses disarm the controller; after a fault, repeat the LOW-to-HIGH neutral arming sequence.

Configure receiver failsafe to drop pulses or drive the arm channel LOW. A receiver that holds valid commands can prevent radio-loss detection. Software stop does not isolate motor power.

## Readiness

The single-file sketch compiled for `arduino:avr:mega` in both default and enabled-control configurations. Embedded checks passed for input scaling, drive mixing, and speed ramps. Hardware operation remains untested; wheel mapping, polarity, and receiver behavior must be checked before driving.

