# Build and flash

Use Python 3.12 or newer and PlatformIO Core 6.2.0. The pinned pioarduino platform requires Core >=6.2.0; older global installations cannot build it.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-dev.txt
.venv/bin/pio run
```

The first build downloads the ESP32 toolchain and Arduino core. PlatformIO/pioarduino also manage shared packages and helper environments under `~/.platformio`; the local virtual environment pins the project's CLI version, not the location of these shared packages.

The environment uses the ESP32-C6 DevKitM 4 MB profile for the selected SuperMini, with USB CDC enabled on boot and GPIO0 as the active-high output. Confirm chip and flash identity during first connection. This profile does not establish the seller board's physical pinout or regulator capacity.

```sh
.venv/bin/pio device list
.venv/bin/pio run -t upload --upload-port /dev/ttyACM0
.venv/bin/pio device monitor --port /dev/ttyACM0
```

Replace the example serial port with the actual port. Do not upload until the physical connections have been checked. For recovery, hold BOOT, press and release RESET, then release BOOT and retry upload. Use a USB data cable. Normal startup must not wait for a serial terminal; the finished firmware runs from a wall supply.
