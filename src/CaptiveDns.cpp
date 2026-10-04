#include "CaptiveDns.h"
#include <cstring>

namespace {
uint16_t read16(const uint8_t* bytes) {
  return static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
}
}

size_t buildCaptiveDnsReply(const uint8_t* request, size_t length,
                           const DnsAddress& address, DnsPacket& response) {
  if (!request || length < 17 || length > response.size()) return 0;
  // Standard, non-truncated query with one question and at most one EDNS OPT.
  if ((request[2] & 0xFA) != 0 || read16(request + 4) != 1
      || read16(request + 6) != 0 || read16(request + 8) != 0
      || read16(request + 10) > 1) return 0;
  size_t cursor = 12;
  while (true) {
    if (cursor >= length) return 0;
    const uint8_t labelLength = request[cursor++];
    if (labelLength == 0) break;
    // Compression pointers and extended label types are not accepted.
    if (labelLength > 63 || labelLength > length - cursor) return 0;
    cursor += labelLength;
    if (cursor - 12 >= 255) return 0;
  }
  if (length - cursor < 4) return 0;
  const uint16_t type = read16(request + cursor);
  if (read16(request + cursor + 2) != 1) return 0;
  const size_t questionEnd = cursor + 4;
  if (read16(request + 10) == 1) {
    // Accept one complete root-owned EDNS(0) OPT, but do not echo it.
    if (length - questionEnd < 11) return 0;
    const auto* opt = request + questionEnd;
    if (opt[0] != 0 || read16(opt + 1) != 41 || opt[6] != 0
        || read16(opt + 9) != length - questionEnd - 11) return 0;
  } else if (questionEnd != length) {
    return 0;
  }
  const bool answer = type == 1 || type == 255;
  if (questionEnd + (answer ? 16 : 0) > response.size()) return 0;
  std::memcpy(response.data(), request, questionEnd);
  response[2] = static_cast<uint8_t>(0x84 | (request[2] & 1)); // QR, AA, original RD
  response[3] = 0; // NOERROR; no recursion or DNSSEC authentication
  response[7] = answer ? 1 : 0;
  response[10] = response[11] = 0;
  if (!answer) return questionEnd;
  // Compressed name points to the question, A/IN, TTL=0, four address bytes.
  const uint8_t record[]{0xC0, 0x0C, 0, 1, 0, 1, 0, 0, 0, 0, 0, 4,
                         address[0], address[1], address[2], address[3]};
  std::memcpy(response.data() + questionEnd, record, sizeof(record));
  return questionEnd + sizeof(record);
}
