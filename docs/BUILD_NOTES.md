# Build preparation and checks

[Back to the project overview](../README.md)

1. Open the self-contained sketch and follow the [firmware setup](../firmware/README.md). No companion headers are needed.
2. With power disconnected, compare the wiring with the [schematic](../hardware/Sch-MT.jpg) and firmware pin reference. Confirm driver modes, motor groups, and polarity.
3. Check converter output and power ratings. Size wiring, connectors, and fuses for the motor loads; PCB current capacity is unverified.
4. With motor power disconnected, verify receiver pulse ranges, arm-switch assignment, and radio-loss behavior.
5. Confirm the settings at the top of the sketch, then set `ROVER_CONFIGURATION_CONFIRMED` to `1` and compile for Mega 2560. Test at limited output with the rover supported and wheels clear.
6. Check each motor's direction, steering, neutral arming, arm-off shutdown, signal-loss shutdown, and re-arming. Verify the physical power disconnect before ground testing.


