#include "ConfigApi.h"
#include <cassert>
#include <iostream>
#include <string>

class Storage : public SettingsStorage {
public:
  bool fail = false;
  int writes = 0;
  bool read(SettingsRecord&) override { return false; }
  bool write(const SettingsRecord&) override { ++writes; return !fail; }
};
int main() {
  Storage storage;
  Configuration config(storage);
  config.begin();
  assert(configResponse(config).body == "{\"message\":\"FELIZ NATAL!\",\"wpm\":25}");
  auto response = saveConfig(config, "message=++hello+++world%21++&wpm=40");
  assert(response.status == 200 && response.restart && response.body == "{\"message\":\"HELLO WORLD!\",\"wpm\":40}");
  assert(saveConfig(config, "wpm=40&message=HELLO+WORLD!").status == 200 && storage.writes == 1);
  assert(saveConfig(config, "message=A%2BB%3DC%26&wpm=25").status == 400);
  assert(saveConfig(config, "message=A%2BB%3DC&wpm=25").status == 200);
  assert(std::string(config.current().message) == "A+B=C");
  const auto previous = config.current();
  for (const char* body : {"", "message=E", "wpm=25", "message=&wpm=25", "message=+++&wpm=25", "message=E&wpm=4", "message=E&wpm=41", "message=E&wpm=-5", "message=E&wpm=5.0", "message=E&wpm=1e1", "message=E&wpm=", "message=E&wpm=123456789", "message=E&wpm=25&wpm=30", "message=E&message=T&wpm=25", "message=E&wpm=25&x=1", "message=E&wpm=25&", "message=%&wpm=25", "message=%GG&wpm=25", "message=E%00E&wpm=25", "message=Ol%C3%A1&wpm=25", "message=%3Cscript%3E&wpm=25", "message=%22E%22&wpm=25"}) {
    response = saveConfig(config, body);
    assert(response.status == 400 && !response.restart);
    assert(sameSettings(previous, config.current()));
  }
  assert(saveConfig(config, std::string(1025, 'a')).status == 413);
  assert(saveConfig(config, "message=" + std::string(121, 'E') + "&wpm=25").status == 400);
  assert(saveConfig(config, "message=" + std::string(120, 'E') + "&wpm=5").status == 200);
  const auto beforeFailure = config.current(); storage.fail = true;
  response = saveConfig(config, "message=SOS&wpm=25");
  assert(response.status == 500 && !response.restart && sameSettings(beforeFailure, config.current()));
  std::cout << "PASS API normalization, form decoding, validation, size limit, duplicate fields, persistence failure and restart contract\n";
}
