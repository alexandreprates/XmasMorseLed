#include "ConfigurationServer.h"
#include "CaptiveDns.h"
#include "WebPage.h"
#include <WiFi.h>
#include <esp_mac.h>

bool ConfigurationServer::begin() {
  if (server) return true;
  const IPAddress address(192, 168, 4, 1);
  const IPAddress mask(255, 255, 255, 0);
  const IPAddress leaseStart(192, 168, 4, 2);
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) != ESP_OK) return false;
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "XmasMorseLed-%02X%02X%02X", mac[3], mac[4], mac[5]);
  // DHCP must advertise this AP as the DNS server for portal detection.
  if (!WiFi.mode(WIFI_AP) || !WiFi.softAPConfig(address, address, mask, leaseStart, address)
      || !WiFi.softAP(ssid)) return failStartup();
  httpd_config_t options = HTTPD_DEFAULT_CONFIG();
  options.task_priority = 1;
  options.stack_size = 6144;
  options.max_open_sockets = 4;
  options.lru_purge_enable = true;
  options.recv_wait_timeout = 5;
  options.send_wait_timeout = 5;
  options.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&server, &options) != ESP_OK) return failStartup();
  httpd_uri_t routes[4]{};
  routes[0].uri = "/"; routes[0].method = HTTP_GET; routes[0].handler = page;
  routes[1].uri = "/api/config"; routes[1].method = HTTP_GET; routes[1].handler = getConfig;
  routes[2].uri = "/api/config"; routes[2].method = HTTP_POST; routes[2].handler = postConfig;
  // Explicit page/API routes take precedence over connectivity probes.
  routes[3].uri = "/*"; routes[3].method = HTTP_GET; routes[3].handler = redirectToPortal;
  for (auto& route : routes) {
    route.user_ctx = this;
    if (httpd_register_uri_handler(server, &route) != ESP_OK) {
      return failStartup();
    }
  }
  const DnsAddress resolvedAddress{address[0], address[1], address[2], address[3]};
  dns.onPacket([resolvedAddress](AsyncUDPPacket& packet) {
    DnsPacket response;
    const size_t length = buildCaptiveDnsReply(packet.data(), packet.length(), resolvedAddress, response);
    if (length) packet.write(response.data(), length);
  });
  if (!dns.listen(address, 53)) {
    dns.close();
    Serial.printf("Captive portal DNS unavailable; use http://192.168.4.1\n");
  }
  Serial.printf("Wi-Fi: %s (open)\nOpen http://192.168.4.1\n", ssid);
  return true;
}

bool ConfigurationServer::failStartup() {
  dns.close();
  if (server) httpd_stop(server);
  server = nullptr;
  WiFi.softAPdisconnect(true);
  return false;
}

esp_err_t ConfigurationServer::redirectToPortal(httpd_req_t* request) {
  httpd_resp_set_status(request, "302 Found");
  // Never reflect the client-supplied Host or URI into the redirect.
  httpd_resp_set_hdr(request, "Location", "http://192.168.4.1/");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  httpd_resp_set_hdr(request, "Connection", "close");
  httpd_resp_set_type(request, "text/plain; charset=utf-8");
  constexpr char message[] = "Abra http://192.168.4.1/ para configurar as luzes.";
  httpd_resp_send(request, message, sizeof(message) - 1);
  // Close without draining any unexpected probe body.
  return ESP_FAIL;
}

esp_err_t ConfigurationServer::respond(httpd_req_t* request, const ApiResponse& response) {
  const char* status = "500 Internal Server Error";
  switch (response.status) {
    case 200: status = "200 OK"; break;
    case 400: status = "400 Bad Request"; break;
    case 408: status = "408 Request Timeout"; break;
    case 413: status = "413 Content Too Large"; break;
    case 503: status = "503 Service Unavailable"; break;
  }
  httpd_resp_set_status(request, status);
  httpd_resp_set_type(request, "application/json; charset=utf-8");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  return httpd_resp_send(request, response.body.data(), response.body.size());
}

esp_err_t ConfigurationServer::reject(httpd_req_t* request, const ApiResponse& response) {
  httpd_resp_set_hdr(request, "Connection", "close");
  respond(request, response);
  // Returning ESP_FAIL closes the session without draining an untrusted body.
  return ESP_FAIL;
}

esp_err_t ConfigurationServer::page(httpd_req_t* request) {
  httpd_resp_set_type(request, "text/html; charset=utf-8");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  return httpd_resp_send(request, WEB_PAGE, sizeof(WEB_PAGE) - 1);
}

esp_err_t ConfigurationServer::getConfig(httpd_req_t* request) {
  const auto& self = *static_cast<ConfigurationServer*>(request->user_ctx);
  return respond(request, configResponse(self.configuration));
}

esp_err_t ConfigurationServer::postConfig(httpd_req_t* request) {
  auto& self = *static_cast<ConfigurationServer*>(request->user_ctx);
  if (request->content_len > MAX_REQUEST_BODY) {
    return reject(request, apiError(413, "body_too_large", "A solicitação excede o limite de 1 KB."));
  }
  char type[80]{};
  if (request->content_len == 0 || httpd_req_get_hdr_value_len(request, "Transfer-Encoding") != 0
      || httpd_req_get_hdr_value_str(request, "Content-Type", type, sizeof(type)) != ESP_OK) {
    return reject(request, apiError(400, "invalid_form", "Envie mensagem e velocidade como formulário."));
  }
  const std::string_view mediaType(type);
  constexpr std::string_view expected = "application/x-www-form-urlencoded";
  if (mediaType != expected && !(mediaType.size() > expected.size()
      && mediaType.substr(0, expected.size()) == expected && mediaType[expected.size()] == ';')) {
    return reject(request, apiError(400, "invalid_form", "Envie um formulário simples, sem arquivos."));
  }
  if (!self.morse.isReady()) {
    return reject(request, apiError(503, "playback_unavailable", "Transmissão indisponível. Reinicie o dispositivo."));
  }
  // Size and media type are checked before any allocation or body read.
  std::string body(request->content_len, '\0');
  size_t received = 0;
  while (received < body.size()) {
    const int count = httpd_req_recv(request, &body[received], body.size() - received);
    if (count <= 0) {
      return reject(request, apiError(408, "incomplete_body", "A conexão foi interrompida. Tente salvar novamente."));
    }
    received += static_cast<size_t>(count);
  }
  auto response = saveConfig(self.configuration, body);
  if (response.restart && !self.morse.apply(self.configuration.current())) {
    response = apiError(503, "playback_unavailable", "Configuração salva. Reinicie o dispositivo para transmitir.");
  }
  return respond(request, response);
}
