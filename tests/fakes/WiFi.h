#pragma once
#include <cstdio>
#include <string>
struct IPAddress { unsigned a,b,c,d; IPAddress(unsigned a,unsigned b,unsigned c,unsigned d):a(a),b(b),c(c),d(d){} };
constexpr int WIFI_AP = 1;
struct FakeWiFi {
  std::string ssid;
  bool mode(int mode) { return mode == WIFI_AP; }
  bool softAPConfig(IPAddress ip, IPAddress, IPAddress) { return ip.a == 192 && ip.b == 168 && ip.c == 4 && ip.d == 1; }
  bool softAP(const char* name) { ssid = name; return true; }
};
struct FakeSerial { template<typename... Args> void printf(const char*, Args...) {} };
extern FakeWiFi WiFi;
extern FakeSerial Serial;
