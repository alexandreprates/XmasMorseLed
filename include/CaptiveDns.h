#ifndef CAPTIVE_DNS_H
#define CAPTIVE_DNS_H

#include <array>
#include <cstddef>
#include <cstdint>

using DnsPacket = std::array<uint8_t, 512>;
using DnsAddress = std::array<uint8_t, 4>;

// Single uncompressed IN question; A/ANY returns the AP IPv4 with TTL zero.
// Other types receive an empty answer. Invalid/unsupported packets are dropped.
size_t buildCaptiveDnsReply(const uint8_t* request, size_t length,
                           const DnsAddress& address, DnsPacket& response);

#endif
