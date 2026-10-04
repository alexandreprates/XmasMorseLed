# Local configuration API

Base URL on the device: `http://192.168.4.1`. The AP is intentionally open, without additional HTTP authentication. Responses and the page use `Cache-Control: no-store`.

## Routes

| Method and path | Behavior |
| --- | --- |
| `GET /` | Self-contained HTML page in Brazilian Portuguese |
| `GET /api/config` | Current saved configuration as JSON |
| `POST /api/config` | Validate, save and restart playback |
| Other `GET` paths | `302 Found` to `http://192.168.4.1/` for captive-portal discovery |

Explicit page/API routes are registered before the GET fallback, so API responses remain JSON and the root page does not redirect to itself. Redirect responses have a fixed absolute `Location`, `Cache-Control: no-store` and `Connection: close`; no client-supplied host/path is reflected. They close without reading unexpected request bodies or touching settings/playback. Unknown POST paths and unsupported methods are not redirected.

Example response:

```json
{"message":"FELIZ NATAL!","wpm":25}
```

Example update:

```sh
curl --data-urlencode 'message=Boas festas!' --data-urlencode 'wpm=12' \
  http://192.168.4.1/api/config
```

POST requires `Content-Type: application/x-www-form-urlencoded` (an optional charset parameter is accepted), a known nonzero Content-Length, and exactly one `message` and one `wpm` field. Body limit: **1024 bytes before form decoding**. Multipart, files and Transfer-Encoding bodies are not accepted. The transport checks declared length and content type before allocating or reading the body, and closes rejected sessions.

`message` accepts at most 120 input ASCII bytes. Validation rejects missing/empty/all-space messages, control bytes, NUL, non-ASCII and punctuation outside the documented table. Successful responses return uppercase text with trimmed/collapsed ASCII spaces. Length is checked before normalization. `wpm` contains decimal digits representing an integer from 5 to 40; fractions and signed/exponent strings are rejected.

On success, POST returns the same configuration shape as GET. It commits before updating memory or queuing playback; a queue of length one keeps the latest successful update. Repeating a save with identical values still restarts playback but skips storage writes. The response confirms that playback was queued, not that a physical pulse has already been measured.

## Errors

Application errors use JSON, for example:

```json
{"error":"unsupported_character","message":"Use letras sem acento, números e a pontuação indicada.","field":"message"}
```

| Status | Meaning |
| --- | --- |
| 400 | Invalid form, missing/duplicate/unknown fields, invalid message or speed |
| 408 | Incomplete or timed-out request body; no configuration change |
| 413 | Body exceeds 1024 bytes |
| 500 | Persistent storage failed; previous settings retained |
| 503 | Playback task unavailable; request rejected before storage, or an unexpected queue error after saving (response explains restart requirement) |

`field` is `message`, `wpm` or an empty string for a general failure. Error text is Portuguese for display by the device's page. Lower-level malformed requests, unknown non-GET paths and unsupported methods may use ESP-IDF's built-in HTTP error responses.

## Captive-portal discovery

DHCP advertises `192.168.4.1` as the DNS server, with leases starting at `192.168.4.2`. Arduino-ESP32 `AsyncUDP` listens on that address at UDP port 53. The portable `CaptiveDns` parser resolves wildcard IPv4 queries to the AP address with TTL zero to avoid retaining those redirects after leaving the AP. DNS processing is asynchronous; no DNS polling or changes to the Morse task are needed.

The parser accepts packets up to 512 bytes with one uncompressed IN question and an optional EDNS(0) OPT record. A/ANY queries receive an A record; other types, including AAAA and HTTPS, receive NOERROR with no answers. Malformed, truncated, compressed-question and unsupported packets are dropped. Input lengths are checked before access, and replies use a fixed-size buffer. This is a small local captive DNS responder, not a recursive resolver or DNSSEC service.

Connectivity probes such as Android's `/generate_204`, Apple's `/hotspot-detect.html` and Windows' `/connecttest.txt` receive the HTTP redirect. The operating system decides whether to open a portal window or show a sign-in notification; behavior can differ across OS versions, reconnects, VPN/private DNS settings and disabled automatic detection. The device does not claim internet access or require a login. Direct `http://192.168.4.1` access remains the fallback. HTTPS is not intercepted, and DHCP captive-portal option 114 is not used.

If DNS startup fails, the firmware logs the failure and keeps the HTTP server/AP available for manual access; restart to retry DNS initialization. Wi-Fi configuration, HTTP startup or route-registration failures tear down the partial server/AP and leave Morse playback independent. Repeated successful `begin()` calls do not create duplicate servers or routes.

The DNS/HTTP approach follows the [Espressif captive-portal example](https://github.com/espressif/esp-idf/tree/v5.5.5/examples/protocols/http_server/captive_portal), with the [bundled AsyncUDP transport](https://github.com/espressif/arduino-esp32/tree/3.3.12/libraries/AsyncUDP). A dedicated bounded parser avoids the unbounded name-terminator search observed in the bundled DNSServer 3.3.12 implementation.

## Persistence

NVS namespace: `xmasmorse`; key: `settings`. One 132-byte blob holds the whole configuration. The record layout is independent of C++ struct padding:

| Bytes | Meaning |
| --- | --- |
| 0–3 | ASCII magic `XML2` |
| 4 | Format version, currently 1 |
| 5 | WPM |
| 6 | Message length (1–120) |
| 7 | Reserved, zero |
| 8–127 | Canonical ASCII message, zero-padded |
| 128–131 | Little-endian CRC-32 of bytes 0–127 |

Loading checks size, magic, version, CRC and canonical content. Unknown formats and corruption use the defaults without silently rewriting Flash. Storage is written only after explicit changed-value saves. This is integrity checking, not encryption. No v1 storage migration is needed: the ATtiny firmware had a compile-time message.

## Implementation choice

The Arduino-ESP32 package includes the native `esp_http_server` used here. Its handler sees `content_len` before body reads and can close a rejected session by returning an error. The Arduino `WebServer` helper was evaluated but not retained because its multipart parsing occurs before application callbacks and does not enforce the required total body bound.

The HTTP task is the sole owner of `Configuration` after setup. It never mutates the Morse engine; it sends copies to `MorseRuntime`. Wi-Fi/task initialization failures are logged over USB Serial, and playback does not depend on a serial terminal or a connected browser.
