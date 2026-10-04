// Preview bridge: the real C++ API with in-memory storage, without ESP32 drivers.
#include "ConfigApi.h"
#include <iostream>
#include <string>

class MemoryStorage : public SettingsStorage {
public:
  bool read(SettingsRecord&) override { return false; }
  bool write(const SettingsRecord&) override { return true; }
};
int main() {
  MemoryStorage storage;
  Configuration configuration(storage);
  configuration.begin();
  std::string command;
  while (std::getline(std::cin, command)) {
    ApiResponse response = configResponse(configuration);
    if (command.rfind("POST ", 0) == 0) {
      std::string body;
      for (size_t i = 5; i + 1 < command.size(); i += 2) {
        body += static_cast<char>(std::stoi(command.substr(i, 2), nullptr, 16));
      }
      response = saveConfig(configuration, body);
    }
    std::cout << response.status << '\t' << response.body << std::endl;
  }
}
