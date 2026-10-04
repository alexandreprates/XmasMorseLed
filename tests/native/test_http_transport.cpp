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
std::vector<httpd_uri_t> routes;
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
esp_err_t httpd_start(httpd_handle_t* server, const httpd_config_t*) { *server = &routes; return ESP_OK; }
esp_err_t httpd_stop(httpd_handle_t) { return ESP_OK; }
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t* route) { routes.push_back(*route); return ESP_OK; }
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
int main() {
  Storage storage; Configuration configuration(storage); MorseRuntime runtime;
  runtime.begin(configuration.current());
  ConfigurationServer server(configuration,runtime);
  assert(server.begin() && routes.size()==3 && WiFi.ssid=="XmasMorseLed-030405");
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
  std::cout<<"PASS production HTTP adapter: reject before read, first error, multipart, truncated bodies, sequential requests, save failure and restart\n";
}
