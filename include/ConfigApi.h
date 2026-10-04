#ifndef CONFIG_API_H
#define CONFIG_API_H

#include "Configuration.h"
#include <string>
#include <string_view>

constexpr size_t MAX_REQUEST_BODY = 1024;
struct ApiResponse {
  int status;
  std::string body;
  bool restart = false;
};

ApiResponse configResponse(const Configuration& configuration);
ApiResponse saveConfig(Configuration& configuration, std::string_view body);
ApiResponse apiError(int status, const char* code, const char* message, const char* field = "");

#endif
