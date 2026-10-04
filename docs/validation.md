# Version 2 validation

Validation date: 2026-10-04. These results describe software checks on the development machine, not a hardware certification.

## Passed

- ESP32-C6 firmware build with PlatformIO 6.2.0, pioarduino 55.03.312-1, Arduino-ESP32 3.3.12 and the bundled ESP-IDF 5.5.5 libraries.
- Four native C++ suites under AddressSanitizer and UndefinedBehaviorSanitizer: Morse engine/table, configuration/persistence, form API and production HTTP adapter with recording transport fakes.
- All 49 non-space table entries decoded against explicit expected patterns; space handling, 1/3/7-unit gaps, 14-unit repetition gap, restart, scheduler delays, rollover and invalid-configuration shutdown.
- Input limits and normalization, maximum-length persistence round trip, corruption of each record byte, unchanged-value write avoidance, failed writes and simulated reboot restore.
- Production HTTP adapter tests: first request rejected with a body, oversized and multipart requests rejected without body reads, partial reads, interrupted bodies, sequential invalid/valid requests and save-failure behavior.
- Chromium browser integration against the real portable C++ API: save/reload, normalization, speed-control synchronization, client/server validation, duplicate-submit protection, storage-error display, lost-connection preservation and initial-load retry.
- Responsive screenshot and overflow checks at 1440×900, 1024×768, 390×844 and 320×740. Page makes no external network requests.
- Editable SVG circuit and perfboard drawings parsed and rendered for visual inspection.
- Independent clean-context reviews for the platform/hardware, Morse core, persistence/runtime and web adapter. Findings were corrected before completion.

The native preview uses in-memory storage. The HTTP transport tests use ESP-IDF API fakes around the production adapter; they do not execute lwIP or radio firmware. The browser uses the full Chromium channel because headless-shell screenshots were unreliable in this environment.

Build report after HTTP integration: app Flash **1,042,650 / 1,310,720 bytes** and static RAM **42,620 / 327,680 bytes**. This is the app partition budget, not the entire 4 MB chip. Runtime heap, Wi-Fi allocations and task stacks need on-device observation.

## Reproduce

```sh
./scripts/test_native.sh
.venv/bin/pio run
.venv/bin/python scripts/preview.py
# In another terminal:
.venv/bin/python tests/browser/check_ui.py
```

Browser screenshots are generated under `test-results/`: `ui-desktop.png`, `ui-tablet.png`, `ui-mobile.png`, `ui-narrow.png`, `ui-offline.png`, `hardware-schematic.png`, and `hardware-perfboard.png`.

## Pending physical acceptance

- Confirm ESP32-C6 chip/4 MB Flash, actual GPIO0 and USB 5 V header mapping on the selected SuperMini.
- Flash over USB; exercise BOOT/RESET recovery and startup from a wall supply without a host.
- Verify the BC337 manufacturer pinout, tree/white-star LED ratings, currents, brightness, temperatures and supply stability. Follow the [hardware checklist](hardware/README.md).
- Confirm tree off during boot/reset and star always on; scope 5/25/40-WPM pulses with Wi-Fi traffic and after a save.
- Connect a real phone to the open AP, including its “no internet” behavior; verify the page and error recovery.
- Save changed settings, physically remove/reapply power, and confirm NVS restoration. Check repeated saves and latest-message application on the device.
- Observe runtime memory and stability during sustained use. Flash writes can stall execution briefly; the new message deliberately restarts after saving.

No firmware upload, radio test, electrical measurement or real Flash fault injection was performed during the automated implementation.
