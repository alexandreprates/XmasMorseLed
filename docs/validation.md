# Version 2 validation

Validation date: 2026-10-04. These results describe software checks on the development machine, not a hardware certification.

## Passed

- ESP32-C6 firmware build with PlatformIO 6.2.0, pioarduino 55.03.312-1, Arduino-ESP32 3.3.12 and the bundled ESP-IDF 5.5.5 libraries.
- Seven native C++ suites under AddressSanitizer and UndefinedBehaviorSanitizer: Morse engine/table, configuration/persistence, form API, captive DNS packets, production HTTP adapter, production Preferences adapter and production Morse runtime.
- All 49 non-space table entries decoded against explicit expected patterns; space handling, 1/3/7-unit gaps, 14-unit repetition gap, restart, scheduler delays, rollover and invalid-configuration shutdown.
- Input limits and normalization, maximum-length persistence round trip, corruption of each record byte, unchanged-value write avoidance, failed writes and simulated reboot restore.
- Binary record compatibility against a golden fixture with an independently calculated CRC; rejection of correctly checksummed records with noncanonical messages, unsupported characters, embedded NUL and invalid speeds without modifying the output settings.
- Production Preferences adapter with a recording fake: namespace open failures, missing/wrong-size records, short reads/writes, handle closure, exact serialized writes, unchanged-save avoidance, restore and corrupt-record fallback.
- Production Morse runtime with a deterministic clock, GPIO recorder and one-slot queue fake: queue/task creation failures and retry, failed-task resource cleanup, duplicate initialization, copied settings, latest-update wins, interruption of an active pulse, restart at the new speed and rejected updates preserving playback.
- Production HTTP adapter tests: first request rejected with a body, oversized and multipart requests rejected without body reads, partial reads, interrupted bodies, sequential invalid/valid requests and save-failure behavior.
- Captive-portal adapter tests: DHCP DNS/lease configuration, wildcard DNS startup and zero TTL, common connectivity-probe redirects, fixed destination despite foreign hosts/paths, no probe body reads or configuration/playback changes, root/API route precedence, startup failure cleanup/retry, duplicate initialization and DNS failure preserving direct HTTP access.
- DNS byte-level tests: exact A response, transaction ID and recursion flag, ANY/AAAA/HTTPS queries, EDNS handling, all truncations of a valid request, missing name terminator, invalid section counts/labels/classes, compressed questions, maximum-length names and deterministic arbitrary datagrams. The production UDP callback is also exercised with valid and malformed packets.
- Chromium browser integration against the real portable C++ API: save/reload, normalization, speed-control synchronization, client/server validation, duplicate-submit protection, storage-error display, lost-connection preservation and initial-load retry.
- Responsive screenshot and overflow checks at 1440×900, 1024×768, 390×844 and 320×740. Page makes no external network requests.
- Editable SVG circuit and perfboard drawings parsed and rendered for visual inspection.
- Independent clean-context reviews for the platform/hardware, Morse core, persistence/runtime and web adapter. Findings were corrected before completion.

The native preview uses in-memory storage. The HTTP transport tests use ESP-IDF, Wi-Fi and AsyncUDP API fakes around the production adapter, with the real DNS packet parser; they do not execute lwIP or radio firmware. Their route dispatcher models literal paths and the trailing wildcard, not the full ESP-IDF HTTP parser. Browser preview checks do not simulate OS captive-portal detection. Runtime tests execute the real worker using a simulated one-millisecond tick and terminate it through the fake scheduler; they do not exercise FreeRTOS concurrency or actual tick timing. Preferences tests exercise the adapter's return-value handling, not physical flash atomicity. The browser uses the full Chromium channel because headless-shell screenshots were unreliable in this environment.

Build report after captive-portal integration: app Flash **1,047,458 / 1,310,720 bytes** and static RAM **42,676 / 327,680 bytes**. This is the app partition budget, not the entire 4 MB chip. Runtime heap, Wi-Fi allocations and task stacks need on-device observation.

## Reproduce

```sh
./scripts/test_native.sh
.venv/bin/pio run
.venv/bin/python scripts/preview.py
# In another terminal:
.venv/bin/python tests/browser/check_ui.py
```

Browser screenshots are generated under `test-results/`: `ui-desktop.png`, `ui-tablet.png`, `ui-mobile.png`, `ui-narrow.png`, `ui-offline.png`, `hardware-schematic.png`, and `hardware-perfboard.png`.

The native runner requires Bash and a C++17 compiler with AddressSanitizer and UndefinedBehaviorSanitizer (GCC on Linux was used). It builds each `tests/native/test_*.cpp` into a temporary directory, stops on any compile/assertion/sanitizer failure and removes its binaries afterward. Set `CXX` to select another compatible compiler. Hardware fakes live under `tests/fakes/` and are only included by the host runner.

## Pending physical acceptance

- Confirm ESP32-C6 chip/4 MB Flash, actual GPIO0 and USB 5 V header mapping on the selected SuperMini.
- Flash over USB; exercise BOOT/RESET recovery and startup from a wall supply without a host.
- Verify the BC337 manufacturer pinout, tree/white-star LED ratings, currents, brightness, temperatures and supply stability. Follow the [hardware checklist](hardware/README.md).
- Confirm tree off during boot/reset and star always on; scope 5/25/40-WPM pulses with Wi-Fi traffic and after a save.
- Connect a real phone to the open AP, including its “no internet” behavior; verify the page and error recovery.
- On Android, iOS and Windows, test first join and reconnect: automatic portal/sign-in notification, saving inside the portal window, and manual `http://192.168.4.1` access when automatic detection does not appear. Confirm staying connected to the local network without internet.
- Confirm a DHCP client receives DNS `192.168.4.1`; while connected, `nslookup example.com 192.168.4.1` should return the AP IPv4 address. `curl -i http://192.168.4.1/generate_204` should return `302` with `Location: http://192.168.4.1/`; the root should return HTML and `/api/config` JSON.
- Save changed settings, physically remove/reapply power, and confirm NVS restoration. Check repeated saves and latest-message application on the device.
- Observe runtime memory and stability during sustained use. Flash writes can stall execution briefly; the new message deliberately restarts after saving.

No firmware upload, radio test, electrical measurement or real Flash fault injection was performed during the automated implementation.
