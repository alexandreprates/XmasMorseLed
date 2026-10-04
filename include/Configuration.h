#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "Settings.h"
#include <array>

// Stable byte format, independent of struct padding and CPU endianness.
using SettingsRecord = std::array<uint8_t, 132>;
SettingsRecord encodeSettings(const Settings& settings);
bool decodeSettings(const SettingsRecord& record, Settings& settings);

class SettingsStorage {
public:
  virtual ~SettingsStorage() = default;
  virtual bool read(SettingsRecord& record) = 0;
  virtual bool write(const SettingsRecord& record) = 0;
};

enum class SaveResult { Saved, Unchanged, Invalid, StorageError };

// Single owner: setup(), then the HTTP/Arduino loop task.
class Configuration {
public:
  explicit Configuration(SettingsStorage& storage) : storage(storage) {}
  bool begin();
  SaveResult save(const Settings& candidate);
  const Settings& current() const { return settings; }
private:
  SettingsStorage& storage;
  Settings settings{};
};

#endif
