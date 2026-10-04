# XmasMorseLed 2.0

Christmas lights that transmit your message in Morse code, configured from a phone over a local Wi-Fi network.

Version 2 targets an **ESP32-C6 SuperMini**, with a **BC337 NPN transistor** switching the tree LEDs and a separate, continuously lit white star. Firmware and page assets are built together. No internet, router, account or filesystem upload is required.

## Use

1. Assemble and verify the [v2 circuit](docs/hardware/README.md), then [build and flash](docs/build.md) the firmware over USB.
2. Power the SuperMini from a regulated 5 V USB supply.
3. Connect your phone or computer to **`XmasMorseLed-XXXXXX`**. The suffix identifies the device; the network has **no password**. Stay connected if the phone reports that it has no internet.
4. Open the captive-portal page offered by the device, or tap its **sign in to network** notification. If it does not appear, open **http://192.168.4.1** manually in a browser.
5. Enter a message, choose a speed and press **Salvar**. The saved values survive power loss. A successful save restarts the message after a short dark interval.

The Wi-Fi network is deliberately open: anyone connected can change the message. The lights continue transmitting when no browser is connected.

Portal opening depends on the phone/computer's operating system and network settings; it is not guaranteed on every connection. The network stays local and has no internet access. HTTPS sites are not redirected. If the portal window closes, the direct HTTP URL remains available while connected to the network.

## Message and timing

- Default: `FELIZ NATAL!`, **25 words per minute**.
- Input: **1–120 ASCII characters**, including letters, numbers, spaces and `. , ? ! - / ( ) : ; = + @`.
- Lowercase is converted to uppercase. Leading/trailing spaces are removed and consecutive spaces are collapsed. Accents, emoji, tabs and unsupported punctuation are rejected rather than silently removed.
- Speed: integer **5–40 WPM**; the Portuguese page labels this **PPM**.
- Timing unit: integer `1200 / WPM` milliseconds. Dot: 1 unit; dash: 3; symbol gap: 1; letter gap: 3; word gap: 7; repeated-message gap: exactly 14.
- Saving interrupts the current message and waits 7 units at the new speed before restarting. Saving unchanged values restarts playback but does not write Flash again.
- Invalid or missing saved data uses the defaults. Failed saves retain the previous configuration.

A FreeRTOS task owns the nonblocking Morse engine. A separate native ESP-IDF HTTP server task validates and saves settings, then sends a value copy through a queue with one slot. Only the latest pending configuration is retained. Flash operations can briefly stall execution; actual pulse timing under Wi-Fi load must be checked on the board.

## Build

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-dev.txt
.venv/bin/pio run
```

The project pins PlatformIO Core **6.2.0** and pioarduino **55.03.312-1** (Arduino-ESP32 3.3.12). The first build downloads the toolchain. The SuperMini uses the ESP32-C6 DevKitM 4 MB profile, GPIO0 and USB CDC; confirm board identity and Flash capacity on first connection.

See [build, upload and recovery instructions](docs/build.md). v1 ATtiny85/ArduinoISP support remains in Git history and is not a v2 build target.

## Test and preview

Native tests need GCC with C++17 support. Seven suites cover the portable engine, configuration, persistence, API, DNS packets and the production HTTP, Preferences and Morse runtime adapters using host fakes, with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
./scripts/test_native.sh
```

Run the exact page against the real portable C++ configuration API on your computer:

```sh
.venv/bin/python scripts/preview.py
# Open http://127.0.0.1:8080
```

The preview has temporary in-memory storage. It does **not** simulate Wi-Fi, captive-portal discovery, physical GPIO, ESP32 scheduling or NVS Flash. Restarting the preview resets its configuration.

With the preview running, exercise the browser flow and capture screenshots:

```sh
.venv/bin/python -m playwright install chromium
.venv/bin/python tests/browser/check_ui.py
```

Screenshots are written to the ignored `test-results/` directory. See [validation results and remaining physical checks](docs/validation.md).

## Project map

| Component | Responsibility |
| --- | --- |
| `MorseTable`, `Settings`, `MorseTransmitter` | Portable encoding, validation and nonblocking pulse state machine |
| `Configuration`, `PreferencesStorage` | Versioned single-record persistence and default/failure behavior |
| `MorseRuntime` | GPIO owner task and latest-value queue |
| `ConfigApi`, `ConfigurationServer` | Form API, bounded native ESP-IDF HTTP adapter and captive-portal DNS/redirects |
| `CaptiveDns` | Bounded portable DNS query parser and captive-portal replies |
| `web/index.html`, `scripts/embed_web.py` | Self-contained Portuguese page, embedded at build time |
| `docs/hardware/` | Editable SVG circuit, perfboard guide, BOM and connections |

[API contract](docs/api.md) · [Hardware guide](docs/hardware/README.md) · [Build guide](docs/build.md)

## Hardware status and boundaries

**The design still needs physical validation.** The vendor PDF contains inconsistent instructions and an unrelated schematic; the physical board's power connections and pinout must be confirmed. Follow the hardware guide before connecting the existing LED string.

No OTA, Bluetooth, router connection, battery management, PCB manufacturing files or alternative light effects are included. The original PNG diagrams in `docs/` depict **v1 only**; use `docs/hardware/` for version 2.

Licensed under [MIT](LICENSE).
