// Run the production ESP-IDF adapter against a recording transport fake.
#include "ConfigurationServer.h"
#include <WiFi.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

FakeWiFi WiFi;
FakeSerial Serial;
FakeDns dnsState;
std::vector<httpd_uri_t> routes;
httpd_config_t httpOptions{};
bool failHttpStart = false;
int failRegistration = -1, httpStarts = 0, httpStops = 0;
int applied = 0;
bool MorseRuntime::begin(const Settings&) { ready = true; return true; }
bool MorseRuntime::apply(const Settings&) { ++applied; return ready; }
struct Request {
  std::string body = "message=SOS&wpm=25";
  std::map<std::string,std::string> headers{{"Content-Type", "application/x-www-form-urlencoded"}};
  std::string status = "200 OK", response, type;
  size_t offset = 0, reads = 0;
  std::map<std::string,std::string> responseHeaders;
};
Request& state(httpd_req_t* request) { return *static_cast<Request*>(request->test_state); }
esp_err_t httpd_start(httpd_handle_t* server, const httpd_config_t* options) {
  ++httpStarts;
  httpOptions = *options;
  *server = failHttpStart ? nullptr : &routes;
  return failHttpStart ? ESP_FAIL : ESP_OK;
}
esp_err_t httpd_stop(httpd_handle_t server) {
  assert(server == &routes); ++httpStops; routes.clear(); return ESP_OK;
}
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t* route) {
  if (static_cast<int>(routes.size()) == failRegistration) return ESP_FAIL;
  routes.push_back(*route); return ESP_OK;
}
bool httpd_uri_match_wildcard(const char* pattern, const char* uri, size_t length) {
  // Model only the two forms used here: literal paths and the trailing /*.
  if (std::strcmp(pattern, "/*") == 0) return length > 0 && uri[0] == '/';
  return std::string_view(pattern) == std::string_view(uri, length);
}
esp_err_t httpd_resp_set_status(httpd_req_t* r, const char* status) { state(r).status=status; return ESP_OK; }
esp_err_t httpd_resp_set_type(httpd_req_t* r, const char* type) { state(r).type=type; return ESP_OK; }
esp_err_t httpd_resp_set_hdr(httpd_req_t* r, const char* key, const char* value) { state(r).responseHeaders[key]=value; return ESP_OK; }
esp_err_t httpd_resp_send(httpd_req_t* r, const char* body, ssize_t size) { assert(size > 0); state(r).response.assign(body,size); return ESP_OK; }
size_t httpd_req_get_hdr_value_len(httpd_req_t* r, const char* key) { return state(r).headers[key].size(); }
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t* r, const char* key, char* value, size_t size) {
  const auto& text=state(r).headers[key];
  if (text.empty() || text.size()+1 > size) return ESP_FAIL;
  std::memcpy(value,text.c_str(),text.size()+1); return ESP_OK;
}
int httpd_req_recv(httpd_req_t* r, char* buffer, size_t size) {
  auto& s=state(r); ++s.reads;
  const size_t count=std::min({size,static_cast<size_t>(3),s.body.size()-s.offset});
  std::memcpy(buffer,s.body.data()+s.offset,count);s.offset+=count;
  return static_cast<int>(count);
}
struct Storage : SettingsStorage {
  int writes=0; bool fail=false;
  bool read(SettingsRecord&) override { return false; }
  bool write(const SettingsRecord&) override { ++writes;return !fail; }
};
int invoke(size_t route, Request& state, size_t length) {
  httpd_req_t request{length,routes[route].user_ctx,&state};
  return routes[route].handler(&request);
}
size_t findRoute(httpd_method_t method, const char* uri) {
  const size_t pathLength = std::strcspn(uri, "?");
  for (size_t i = 0; i < routes.size(); ++i) {
    if (routes[i].method == method && httpOptions.uri_match_fn(routes[i].uri, uri, pathLength)) return i;
  }
  return routes.size();
}

void testStartupFailures() {
  // Each iteration models a fresh boot followed by an initialization retry.
  for (int failure = 0; failure < 8; ++failure) {
    WiFi = {}; dnsState = {}; routes.clear(); httpStarts = 0; httpStops = 0;
    WiFi.failMode = failure == 0;
    WiFi.failConfig = failure == 1;
    WiFi.failAP = failure == 2;
    failHttpStart = failure == 3;
    failRegistration = failure >= 4 ? failure - 4 : -1;
    Storage storage; Configuration configuration(storage); MorseRuntime runtime;
    ConfigurationServer server(configuration, runtime);
    assert(!server.begin());
    assert(!WiFi.running && WiFi.disconnects == 1 && routes.empty());
    assert(!dnsState.running && dnsState.starts == 0);
    assert(httpStops == (failure >= 4 ? 1 : 0));
    WiFi.failMode = WiFi.failConfig = WiFi.failAP = failHttpStart = false;
    failRegistration = -1;
    assert(server.begin() && WiFi.running && dnsState.running && routes.size() == 4);
  }
  // DNS bind failure must leave the direct HTTP URL usable.
  WiFi = {}; dnsState = {}; routes.clear(); httpStarts = 0; httpStops = 0;
  dnsState.failStart = true;
  Storage storage; Configuration configuration(storage); MorseRuntime runtime;
  ConfigurationServer server(configuration, runtime);
  assert(server.begin() && WiFi.running && !dnsState.running && httpStops == 0);
  assert(dnsState.starts == 1 && dnsState.stops == 1);
  Request page; assert(invoke(findRoute(HTTP_GET, "/"), page, 0) == ESP_OK);
  assert(page.type == "text/html; charset=utf-8");
  Request get; assert(invoke(findRoute(HTTP_GET, "/api/config"), get, 0) == ESP_OK);
  assert(get.response.find("FELIZ NATAL!") != std::string::npos);
}

void testPortal(ConfigurationServer& server, Storage& storage) {
  assert(WiFi.address == IPAddress(192,168,4,1) && WiFi.gateway == WiFi.address);
  assert(WiFi.mask == IPAddress(255,255,255,0) && WiFi.leaseStart == IPAddress(192,168,4,2));
  assert(WiFi.dns == WiFi.address && dnsState.address == WiFi.address);
  assert(dnsState.port == 53 && dnsState.running && dnsState.handler);
  AsyncUDPPacket query{{0x12,0x34,1,0,0,1,0,0,0,0,0,0,1,'a',0,0,1,0,1}, {}};
  dnsState.handler(query);
  assert(query.response.size() == query.request.size() + 16);
  assert(query.response[0] == 0x12 && query.response[1] == 0x34 && query.response[7] == 1);
  assert(std::vector<uint8_t>(query.response.end() - 4, query.response.end()) == std::vector<uint8_t>({192,168,4,1}));
  AsyncUDPPacket malformed{{0x12,0x34,1,0,0,1,0,0,0,0,0,0,1,'a',1,'b',1}, {}};
  dnsState.handler(malformed);
  assert(malformed.response.empty());
  const int starts = httpStarts, dnsStarts = dnsState.starts;
  assert(server.begin() && httpStarts == starts && dnsState.starts == dnsStarts && routes.size() == 4);
  assert(httpOptions.uri_match_fn == httpd_uri_match_wildcard);
  for (const char* path : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/library/test/success.html",
                           "/connecttest.txt", "/ncsi.txt", "/redirect", "/canonical.html",
                           "/unknown?next=https://example.com", "//example.com/"}) {
    Request probe;
    probe.headers["Host"] = "connectivitycheck.example.com";
    assert(invoke(findRoute(HTTP_GET, path), probe, 0) == ESP_FAIL);
    assert(probe.status == "302 Found" && probe.responseHeaders["Location"] == "http://192.168.4.1/");
    assert(probe.responseHeaders["Cache-Control"] == "no-store");
    assert(probe.responseHeaders["Connection"] == "close" && probe.reads == 0);
    assert(probe.type == "text/plain; charset=utf-8" && !probe.response.empty());
  }
  Request withBody;
  assert(invoke(findRoute(HTTP_GET, "/generate_204"), withBody, 100000) == ESP_FAIL);
  assert(withBody.reads == 0 && withBody.status == "302 Found");
  assert(storage.writes == 0 && applied == 0);
  assert(findRoute(HTTP_GET, "/?source=portal") == 0);
  assert(findRoute(HTTP_GET, "/api/config") == 1);
  assert(findRoute(HTTP_POST, "/api/config") == 2);
  assert(findRoute(HTTP_POST, "/generate_204") == routes.size());
  Request root;
  assert(invoke(findRoute(HTTP_GET, "/"), root, 0) == ESP_OK && root.status == "200 OK");
  assert(root.responseHeaders.count("Location") == 0 && root.type == "text/html; charset=utf-8");
  Request get;
  assert(invoke(findRoute(HTTP_GET, "/api/config"), get, 0) == ESP_OK);
  assert(get.responseHeaders.count("Location") == 0 && get.type == "application/json; charset=utf-8");
}

int main() {
  testStartupFailures();
  WiFi = {}; dnsState = {}; routes.clear(); httpStarts = 0; httpStops = 0;
  Storage storage; Configuration configuration(storage); MorseRuntime runtime;
  runtime.begin(configuration.current());
  ConfigurationServer server(configuration,runtime);
  assert(server.begin() && routes.size()==4 && WiFi.ssid=="XmasMorseLed-030405");
  testPortal(server, storage);
  // First-ever request may be invalid: still return a nonempty error response.
  Request huge; assert(invoke(2,huge,1025)==ESP_FAIL && huge.reads==0);
  assert(huge.status=="413 Content Too Large" && huge.response.find("body_too_large")!=std::string::npos);
  assert(huge.responseHeaders["Connection"]=="close");
  Request multipart; multipart.headers["Content-Type"]="multipart/form-data; boundary=x";
  assert(invoke(2,multipart,100)==ESP_FAIL && multipart.reads==0 && multipart.status=="400 Bad Request");
  Request hugeMultipart; hugeMultipart.headers=multipart.headers;
  assert(invoke(2,hugeMultipart,100000)==ESP_FAIL && hugeMultipart.reads==0 && hugeMultipart.status=="413 Content Too Large");
  Request chunked; chunked.headers["Transfer-Encoding"]="chunked";
  assert(invoke(2,chunked,100)==ESP_FAIL && chunked.reads==0);
  Request valid; assert(invoke(2,valid,valid.body.size())==ESP_OK && valid.status=="200 OK");
  assert(valid.reads>1 && applied==1 && storage.writes==1 && valid.response=="{\"message\":\"SOS\",\"wpm\":25}");
  Request again; assert(invoke(2,again,again.body.size())==ESP_OK && applied==2 && storage.writes==1);
  Request truncated; truncated.body="message=E";
  assert(invoke(2,truncated,100)==ESP_FAIL && truncated.status=="408 Request Timeout");
  assert(applied==2 && storage.writes==1);
  Request bad; bad.body="message=%00&wpm=25";
  assert(invoke(2,bad,bad.body.size())==ESP_OK && bad.status=="400 Bad Request");
  storage.fail=true;
  Request failure; failure.body="message=NEW&wpm=40";
  assert(invoke(2,failure,failure.body.size())==ESP_OK && failure.status=="500 Internal Server Error" && applied==2);
  Request get; assert(invoke(1,get,0)==ESP_OK && get.response.find("SOS")!=std::string::npos);
  Request page; assert(invoke(0,page,0)==ESP_OK && page.type=="text/html; charset=utf-8");
  std::cout<<"PASS production HTTP adapter: captive portal, DNS setup/failure, startup cleanup/retry, route priority, bounded bodies, save failure and restart\n";
}
