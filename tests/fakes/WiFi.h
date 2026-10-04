#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
struct IPAddress {
  unsigned a,b,c,d;
  IPAddress(unsigned a=0,unsigned b=0,unsigned c=0,unsigned d=0):a(a),b(b),c(c),d(d){}
  bool operator==(const IPAddress& other) const { return a==other.a && b==other.b && c==other.c && d==other.d; }
  uint8_t operator[](size_t index) const { const unsigned bytes[]{a,b,c,d}; return static_cast<uint8_t>(bytes[index]); }
};
constexpr int WIFI_AP = 1;
struct FakeWiFi {
  std::string ssid;
  bool failMode=false, failConfig=false, failAP=false, running=false;
  int disconnects=0;
  IPAddress address, gateway, mask, leaseStart, dns;
  bool mode(int mode) { return mode == WIFI_AP && !failMode; }
  bool softAPConfig(IPAddress ip, IPAddress gw, IPAddress subnet, IPAddress lease, IPAddress dnsServer) {
    address=ip; gateway=gw; mask=subnet; leaseStart=lease; dns=dnsServer;
    return !failConfig;
  }
  bool softAP(const char* name) { ssid=name; running=!failAP; return running; }
  bool softAPdisconnect(bool off) { ++disconnects; if (off) running=false; return true; }
};
struct FakeSerial { template<typename... Args> void printf(const char*, Args...) {} };
extern FakeWiFi WiFi;
extern FakeSerial Serial;
