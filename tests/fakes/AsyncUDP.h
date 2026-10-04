#pragma once
#include <WiFi.h>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

struct AsyncUDPPacket {
  std::vector<uint8_t> request, response;
  uint8_t* data() { return request.data(); }
  size_t length() { return request.size(); }
  size_t write(const uint8_t* bytes, size_t size) {
    response.assign(bytes, bytes + size);
    return size;
  }
};
struct FakeDns {
  bool failStart = false, running = false;
  int starts = 0, stops = 0;
  uint16_t port = 0;
  IPAddress address;
  std::function<void(AsyncUDPPacket&)> handler;
};
extern FakeDns dnsState;

class AsyncUDP {
public:
  void onPacket(std::function<void(AsyncUDPPacket&)> handler) { dnsState.handler = std::move(handler); }
  bool listen(const IPAddress address, uint16_t port) {
    ++dnsState.starts;
    dnsState.port = port;
    dnsState.address = address;
    dnsState.running = !dnsState.failStart;
    return dnsState.running;
  }
  void close() { ++dnsState.stops; dnsState.running = false; }
};
