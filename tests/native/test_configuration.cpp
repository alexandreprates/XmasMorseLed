#include "Configuration.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

// CRC values independently generated using Python zlib.crc32 over the 128-byte
// XML2 header/payload. Do not derive expected fixtures with encodeSettings().
SettingsRecord fixture(std::string_view message, uint8_t wpm, uint32_t crc) {
  SettingsRecord record{'X', 'M', 'L', '2', 1, wpm,
                        static_cast<uint8_t>(message.size()), 0};
  std::memcpy(record.data() + 8, message.data(), message.size());
  for (size_t i = 0; i < 4; ++i) record[128 + i] = static_cast<uint8_t>(crc >> (8 * i));
  return record;
}

void testRecordCompatibilityAndSemanticValidation() {
  const auto golden = fixture("SOS", 25, 0x8E8BE4BD);
  Settings expected;
  assert(normalizeSettings("SOS", 25, expected) == SettingsError::None);
  assert(encodeSettings(expected) == golden);
  Settings decoded;
  assert(decodeSettings(golden, decoded) && sameSettings(expected, decoded));
  const SettingsRecord invalidRecords[]{
      fixture("e", 25, 0x25E67786),
      fixture(" E", 25, 0x6F661753),
      fixture("E ", 25, 0xEA77C676),
      fixture("E  E", 25, 0xA378EF70),
      fixture("#", 25, 0x023A0B31),
      fixture(std::string_view("\0", 1), 25, 0xFC6CB64A),
      fixture("E", 0, 0x14B2465C),
      fixture("E", 41, 0x934393BF),
  };
  for (const auto& invalid : invalidRecords) {
    assert(!decodeSettings(invalid, decoded));
    assert(sameSettings(decoded, expected));
  }
  Settings invalid = expected;
  invalid.wpm = 0;
  assert(encodeSettings(invalid) == SettingsRecord{});
  invalid = expected; invalid.message[0] = 's';
  assert(encodeSettings(invalid) == SettingsRecord{});
}

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
  testRecordCompatibilityAndSemanticValidation();
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
  std::cout << "PASS persistence, reboot, unchanged writes, failed commit, golden record, semantic validation and 120-character round trip\n";
}
