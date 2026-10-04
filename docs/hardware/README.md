# Version 2 hardware

This is a **reference perfboard design, not a bench-validated circuit**. The selected board is the ESP32-C6 SuperMini shown in the user-supplied `esp32-c6-supermini.pdf` listing (AliExpress item 1005007937264166). Its pictures show GPIO0, USB-C, 5V, GND and 3V3. The listing also contains C3 instructions and an unrelated schematic: do not use that schematic to establish connectivity or regulator ratings.

- [Editable circuit schematic](schematic.svg)
- [Perfboard placement and wiring guide](perfboard.svg)

## Power and switching

Use one regulated **5 V, at least 1 A USB supply**, connected to the SuperMini USB-C socket. Verify that the header pad marked **5V** supplies USB VBUS before connecting the LED circuit. Do not connect a second supply to this pad while USB is attached. Leave battery pads unused. Do not connect 5 V to GPIO0 or 3V3.

The board's regulator powers the ESP32. The LED branches use the USB 5 V rail and share GND with the board; they do not draw power from its 3V3 regulator. No external regulator is required. Keep wires short, and keep metal, wiring and the perfboard ground wiring away from the SuperMini antenna end.

GPIO0 drives Q1 through R1. R2 keeps the base low and Q1 off while the controller is resetting. Q1 switches the tree's negative terminal to ground. The star has its own resistor and stays on whenever power is present. A GPIO HIGH turns the tree on.

## Bill of materials

| Ref | Part | Requirement |
| --- | --- | --- |
| U1 | ESP32-C6 SuperMini | Selected USB-C board; verify ESP32-C6 and 4 MB flash on first connection |
| Q1 | BC337 NPN transistor | TO-92 through-hole; verify C/B/E pinout for the manufacturer |
| R1 | 470 ohm | 1/4 W; series base resistor |
| R2 | 100 kohm | 1/4 W; base-to-emitter pull-down |
| R3 | 56 ohm | 1/2 W; tree series resistor |
| R4 | 220 ohm | 1/4 W; star series resistor |
| C1 | 100 nF ceramic | At least 10 V; across 5 V and GND near the LED branch |
| C2 | 47 uF electrolytic | At least 10 V; positive to 5 V, negative to GND |
| J1 | Two-position terminal | Tree LED connection |
| J2 | Two-position terminal | Star LED connection |
| LED load | Existing tree string | Approximately 3 V / 50 mA; verify actual current |
| Star | White indicator LED | Assumes a conventional low-power indicator, not a high-power LED module |
| Supply | USB 5 V / 1 A or greater | Single supply with a data-capable USB cable for programming |
| Assembly | Isolated-pad perfboard | Female headers for U1, wire, terminal blocks |

Use BC337 from a documented manufacturer. The [onsemi BC337 datasheet](https://www.onsemi.com/pdf/datasheet/bc337-fsc-d.pdf) identifies 1=collector, 2=base, 3=emitter; follow its package drawing rather than assuming every TO-92 transistor has the same order. R1 supplies approximately `(3.3 - 0.8) / 470 = 5.3 mA` of base current for a load below 50 mA. Saturation and the GPIO output voltage must be checked on the actual circuit.

With a 3.0 V tree drop at 5.0 V, R3 gives approximately `(5 - 3 - 0.2) / 56 = 32 mA` (assuming 0.2 V collector-emitter saturation), dissipating about 0.058 W. This is a nominal calculation, not a guaranteed load current: verify the actual string before continuous operation. Use the specified 56-ohm series resistor with the 5 V rail. R4 nominally gives 9 mA with a 3 V white LED. Confirm the star's actual rating; do not apply this circuit to an unidentified high-power LED.

## Wiring table

| Net | Connected terminals |
| --- | --- |
| USB_5V | U1 verified 5V pad, R3 input, R4 input, C1 pin 1, C2 positive |
| GND | U1 GND, Q1 emitter, R2 ground end, J2 star negative, C1 pin 2, C2 negative |
| MORSE | U1 GPIO0, R1 input |
| BASE | R1 output, Q1 base, R2 base end |
| TREE_POS | R3 output, J1 tree positive |
| TREE_NEG | J1 tree negative, Q1 collector |
| STAR_POS | R4 output, J2 star positive |

The perfboard diagram is a **net-labeled placement guide**, not a copper-layer drawing or a board footprint. Every pad on isolated-pad perfboard is separate. Join the connections in this table with insulated wire; route crossings without electrical contact. Mount the SuperMini in sockets with USB-C accessible and its antenna beyond the carrier edge. Confirm the header pitch and spacing from the physical board before soldering sockets.

## Why the external transistor remains

The existing string is estimated at 50 mA. ESP32-C6 datasheet Table 5-4 gives **typical** GPIO source current of 40 mA at VOH >= 2.64 V, and sink current of 28 mA at VOL = 0.495 V, with the strongest driver setting. These are characterization points, not a guaranteed continuous-current budget for powering the entire string. The 3V3 power header is not software-switchable, and the seller's regulator capacity is unverified. Keep BC337 so the GPIO only supplies base current. An individual low-current indicator LED would be a different load.

Source: [Espressif ESP32-C6 datasheet, Table 5-4](https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf).

## Required bench acceptance (pending)

1. With power disconnected, verify polarity, resistor values and no short between 5V and GND. Verify Q1 collector/base/emitter against its datasheet.
2. With the LED branches disconnected, power U1 by USB. Confirm the header 5V/GND mapping, 3.3 V regulation and the selected GPIO0 pad. Confirm chip identity and 4 MB flash using the upload tool. Do not infer these facts from the seller's unrelated schematic.
3. Connect the load branches and measure their on-state currents. Tree current must stay at or below its verified rating (50 mA provisional ceiling); star current must stay below its actual rating. If either rating is unknown or exceeded, disconnect that branch and re-dimension its resistor before approval. The supplied values assume the loads described above.
4. Check supply stability with Wi-Fi active and both branches lit, resistor/transistor temperature and acceptable brightness. Do not approve a circuit that browns out or overheats.
5. Confirm tree off during reset/boot and the star continuously on. Capture GPIO0 pulses at 5, 25 and 40 WPM while loading and saving the page. Flash writes can briefly stall execution; saving deliberately restarts the message.
6. Check startup from a wall supply without a USB host and BOOT/RESET recovery. Record measurements and equipment before marking physical validation complete.

No battery, mains wiring, PCB fabrication files or simultaneous external/USB power are included.
