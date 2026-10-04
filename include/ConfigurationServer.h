#ifndef CONFIGURATION_SERVER_H
#define CONFIGURATION_SERVER_H

#include "ConfigApi.h"
#include "MorseRuntime.h"
#include <esp_http_server.h>

class ConfigurationServer {
public:
  ConfigurationServer(Configuration& configuration, MorseRuntime& morse)
      : configuration(configuration), morse(morse) {}
  bool begin();
private:
  static esp_err_t page(httpd_req_t* request);
  static esp_err_t getConfig(httpd_req_t* request);
  static esp_err_t postConfig(httpd_req_t* request);
  static esp_err_t respond(httpd_req_t* request, const ApiResponse& response);
  static esp_err_t reject(httpd_req_t* request, const ApiResponse& response);
  Configuration& configuration;
  MorseRuntime& morse;
  httpd_handle_t server = nullptr;
};

#endif
