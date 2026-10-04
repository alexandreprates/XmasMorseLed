#include "CaptiveDns.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

namespace {
const DnsAddress address{192, 168, 4, 1};
const std::vector<uint8_t> query{
    0x12,0x34,1,0,0,1,0,0,0,0,0,0,
    3,'w','w','w',7,'e','x','a','m','p','l','e',3,'c','o','m',0,0,1,0,1};

void rejected(const std::vector<uint8_t>& packet) {
  DnsPacket response; response.fill(0xA5);
  const auto previous = response;
  assert(buildCaptiveDnsReply(packet.data(), packet.size(), address, response) == 0);
  assert(response == previous);
}

void testResponses() {
  DnsPacket response;
  const std::vector<uint8_t> expected{
      0x12,0x34,0x85,0,0,1,0,1,0,0,0,0,
      3,'w','w','w',7,'e','x','a','m','p','l','e',3,'c','o','m',0,0,1,0,1,
      0xC0,0x0C,0,1,0,1,0,0,0,0,0,4,192,168,4,1};
  assert(buildCaptiveDnsReply(query.data(), query.size(), address, response) == expected.size());
  assert(std::equal(expected.begin(), expected.end(), response.begin()));

  auto other = query;
  other[0] = 0xAB; other[1] = 0xCD; other[2] = 0;
  other[query.size() - 3] = 255; // ANY still returns an A record.
  assert(buildCaptiveDnsReply(other.data(), other.size(), address, response) == expected.size());
  assert(response[0] == 0xAB && response[1] == 0xCD && response[2] == 0x84 && response[7] == 1);
  assert(response[query.size() + 3] == 1);

  for (const uint8_t type : {28, 65, 15}) { // AAAA, HTTPS, MX: NOERROR without an answer.
    other = query; other[query.size() - 3] = type;
    assert(buildCaptiveDnsReply(other.data(), other.size(), address, response) == query.size());
    assert(response[3] == 0 && response[6] == 0 && response[7] == 0);
    assert(response[query.size() - 3] == type);
  }

  auto edns = query;
  edns[11] = 1;
  edns.insert(edns.end(), {0,0,41,4,208,0,0,0,0,0,0});
  assert(buildCaptiveDnsReply(edns.data(), edns.size(), address, response) == expected.size());
  assert(std::equal(expected.begin(), expected.end(), response.begin()));
  edns[query.size() + 6] = 1; // Only EDNS version zero is supported.
  rejected(edns);
  edns[query.size() + 6] = 0;
  edns.back() = 4; // Declared OPT data must fit in the packet.
  rejected(edns);
  edns.insert(edns.end(), {0,12,0,0}); // Complete empty EDNS padding option.
  assert(buildCaptiveDnsReply(edns.data(), edns.size(), address, response) == expected.size());
}

void testMalformedPackets() {
  DnsPacket response;
  assert(buildCaptiveDnsReply(nullptr, query.size(), address, response) == 0);
  for (size_t size = 0; size < query.size(); ++size)
    rejected(std::vector<uint8_t>(query.begin(), query.begin() + size));
  for (const auto& mutation : std::vector<std::pair<size_t,uint8_t>>{
           {2,0x81}, {2,0x09}, {2,0x03}, // response, non-query opcode, truncated query
           {5,0}, {5,2}, {7,1}, {9,1}, {11,2}, // invalid section counts
           {12,64}, {12,0xC0}, {query.size() - 1,3}}) {
    auto invalid = query; invalid[mutation.first] = mutation.second;
    rejected(invalid);
  }
  auto extra = query; extra.push_back(0);
  rejected(extra);
  auto missingOpt = query; missingOpt[11] = 1;
  rejected(missingOpt);
  // Regression: no NUL after the header, allocated to the exact received size.
  std::vector<uint8_t> unterminated(query.begin(), query.begin() + 12);
  unterminated.insert(unterminated.end(), {1,'a',1,'b',1});
  rejected(unterminated);
  rejected(std::vector<uint8_t>(513, 0xFF));
}

void testNameLimitsAndPacketBounds() {
  DnsPacket response;
  auto longest = std::vector<uint8_t>(query.begin(), query.begin() + 12);
  for (const size_t length : {63,63,63,61}) {
    longest.push_back(static_cast<uint8_t>(length));
    longest.insert(longest.end(), length, 'a');
  }
  longest.insert(longest.end(), {0,0,1,0,1});
  assert(buildCaptiveDnsReply(longest.data(), longest.size(), address, response) == longest.size() + 16);
  longest[204] = 62; // Increase the last label and exceed the 255-byte name limit.
  longest.insert(longest.end() - 5, 'a');
  rejected(longest);

  // Deterministic arbitrary datagrams exercise bounds under ASan/UBSan.
  std::mt19937 random(0xC6);
  for (size_t size = 0; size <= 513; ++size) {
    std::vector<uint8_t> packet(size);
    for (auto& byte : packet) byte = static_cast<uint8_t>(random());
    if (size >= 12) std::copy(query.begin(), query.begin() + 12, packet.begin());
    const size_t count = buildCaptiveDnsReply(packet.data(), packet.size(), address, response);
    assert(count <= response.size());
  }
}
} // namespace

int main() {
  testResponses();
  testMalformedPackets();
  testNameLimitsAndPacketBounds();
  std::cout << "PASS captive DNS packets: exact A answer, ANY, empty AAAA/HTTPS, EDNS, malformed/truncated input and size limits\n";
}
