#include "Configuration.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

struct MemoryStorage : SettingsStorage {
  SettingsRecord saved{};
  bool exists = false;
  bool fail = false;
  int writes = 0;
  bool read(SettingsRecord& record) override { record = saved; return exists && !fail; }
  bool write(const SettingsRecord& record) override {
    ++writes;
    if (fail) return false;
    saved = record; exists = true; return true;
  }
};
int main() {
  MemoryStorage storage;
  Configuration configuration(storage);
  assert(!configuration.begin());
  assert(std::strcmp(configuration.current().message, "FELIZ NATAL!") == 0);
  assert(configuration.current().wpm == 25);
  assert(configuration.save(Settings{}) == SaveResult::Unchanged && storage.writes == 0);
  Settings settings;
  assert(normalizeSettings("  hello   world!  ", 5, settings) == SettingsError::None);
  assert(configuration.save(settings) == SaveResult::Saved && storage.writes == 1);
  assert(configuration.save(settings) == SaveResult::Unchanged && storage.writes == 1);
  Configuration reboot(storage);
  assert(reboot.begin() && sameSettings(settings, reboot.current()));
  const Settings previous = configuration.current();
  settings.wpm = 40; storage.fail = true;
  assert(configuration.save(settings) == SaveResult::StorageError);
  assert(sameSettings(previous, configuration.current()));
  storage.fail = false;
  Configuration afterFailure(storage);
  assert(afterFailure.begin() && sameSettings(previous, afterFailure.current()));
  assert(configuration.save(settings) == SaveResult::Saved);
  assert(configuration.current().wpm == 40);
  settings.wpm = 0;
  const int writes = storage.writes;
  assert(configuration.save(settings) == SaveResult::Invalid && storage.writes == writes);
  assert(normalizeSettings(std::string(120, 'Z'), 40, settings) == SettingsError::None);
  SettingsRecord record = encodeSettings(settings);
  Settings decoded;
  assert(decodeSettings(record, decoded) && sameSettings(settings, decoded));
  // Detect every single-byte corruption of the fixed record, including CRC.
  for (size_t i = 0; i < record.size(); ++i) {
    auto damaged = record; damaged[i] ^= 0x01;
    assert(!decodeSettings(damaged, decoded));
  }
  storage.saved[4] = 99;
  assert(!configuration.begin() && sameSettings(configuration.current(), Settings{}));
  storage.exists = false;
  assert(!configuration.begin());
  std::cout << "PASS persistence, reboot, unchanged writes, failed commit, invalid record and 120-character round trip\n";
}
