#include "PreferencesStorage.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace {
constexpr size_t ALL_BYTES = std::numeric_limits<size_t>::max();
struct FakeNvs {
  std::vector<uint8_t> bytes;
  bool failOpen = false, open = false, readOnly = false;
  size_t readLimit = ALL_BYTES, writeLimit = ALL_BYTES;
  int begins = 0, ends = 0, reads = 0, writes = 0;
} nvs;

void testReadFailures() {
  nvs = {};
  PreferencesStorage storage;
  SettingsRecord record{};
  nvs.failOpen = true;
  assert(!storage.read(record));
  assert(nvs.begins == 1 && nvs.ends == 0 && nvs.reads == 0);
  nvs.failOpen = false;
  for (const size_t size : {size_t{0}, record.size() - 1, record.size() + 1}) {
    nvs.bytes.assign(size, 0);
    const int ends = nvs.ends;
    assert(!storage.read(record));
    assert(!nvs.open && nvs.ends == ends + 1 && nvs.reads == 0);
  }
  const auto encoded = encodeSettings(Settings{});
  nvs.bytes.assign(encoded.begin(), encoded.end());
  for (const size_t limit : {size_t{0}, record.size() - 1}) {
    nvs.readLimit = limit;
    const int ends = nvs.ends;
    assert(!storage.read(record));
    assert(!nvs.open && nvs.ends == ends + 1);
    Configuration configuration(storage);
    assert(!configuration.begin() && sameSettings(configuration.current(), Settings{}));
  }
  nvs.readLimit = ALL_BYTES;
  assert(storage.read(record) && record == encoded && !nvs.open);
}

void testWriteFailuresAndRestore() {
  nvs = {};
  PreferencesStorage storage;
  Configuration configuration(storage);
  Settings initial;
  assert(normalizeSettings("SOS", 5, initial) == SettingsError::None);
  assert(configuration.save(initial) == SaveResult::Saved);
  const auto encoded = encodeSettings(initial);
  assert(nvs.bytes == std::vector<uint8_t>(encoded.begin(), encoded.end()));
  assert(!nvs.open && nvs.begins == nvs.ends);
  const int writes = nvs.writes, opens = nvs.begins;
  assert(configuration.save(initial) == SaveResult::Unchanged);
  assert(nvs.writes == writes && nvs.begins == opens);

  Settings next;
  assert(normalizeSettings("NEW", 40, next) == SettingsError::None);
  nvs.failOpen = true;
  assert(configuration.save(next) == SaveResult::StorageError);
  assert(nvs.writes == writes && sameSettings(configuration.current(), initial));
  nvs.failOpen = false;
  for (const size_t limit : {size_t{0}, encoded.size() - 1}) {
    nvs.writeLimit = limit;
    const int ends = nvs.ends;
    assert(configuration.save(next) == SaveResult::StorageError);
    assert(!nvs.open && nvs.ends == ends + 1);
    assert(sameSettings(configuration.current(), initial));
  }
  nvs.writeLimit = ALL_BYTES;
  assert(configuration.save(next) == SaveResult::Saved);
  Configuration reboot(storage);
  assert(reboot.begin() && sameSettings(reboot.current(), next));
  nvs.bytes[8] ^= 1;
  assert(!reboot.begin() && sameSettings(reboot.current(), Settings{}));
}
} // namespace

bool Preferences::begin(const char* name, bool readOnly) {
  assert(!nvs.open && std::strcmp(name, "xmasmorse") == 0);
  ++nvs.begins;
  if (nvs.failOpen) return false;
  nvs.open = true;
  nvs.readOnly = readOnly;
  return true;
}
size_t Preferences::getBytesLength(const char* key) {
  assert(nvs.open && nvs.readOnly && std::strcmp(key, "settings") == 0);
  return nvs.bytes.size();
}
size_t Preferences::getBytes(const char* key, void* buffer, size_t size) {
  assert(nvs.open && nvs.readOnly && std::strcmp(key, "settings") == 0);
  ++nvs.reads;
  const size_t count = std::min({size, nvs.bytes.size(), nvs.readLimit});
  std::memcpy(buffer, nvs.bytes.data(), count);
  return count;
}
size_t Preferences::putBytes(const char* key, const void* buffer, size_t size) {
  assert(nvs.open && !nvs.readOnly && std::strcmp(key, "settings") == 0);
  ++nvs.writes;
  const size_t count = std::min(size, nvs.writeLimit);
  // The fake reports adapter-level failures; it does not emulate flash atomicity.
  if (count == size) {
    const auto* bytes = static_cast<const uint8_t*>(buffer);
    nvs.bytes.assign(bytes, bytes + size);
  }
  return count;
}
void Preferences::end() {
  assert(nvs.open);
  nvs.open = false;
  ++nvs.ends;
}

int main() {
  testReadFailures();
  testWriteFailuresAndRestore();
  std::cout << "PASS production Preferences adapter: open failure, size mismatch, short reads/writes, cleanup, unchanged writes and restore\n";
}
