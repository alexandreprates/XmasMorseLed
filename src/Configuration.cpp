#include "Configuration.h"
#include <cstring>

namespace {
uint32_t checksum(const SettingsRecord& record) {
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0; i < 128; ++i) {
    crc ^= record[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0U);
    }
  }
  return ~crc;
}
}

SettingsRecord encodeSettings(const Settings& settings) {
  SettingsRecord record{};
  if (!isCanonical(settings)) return record;
  record[0] = 'X'; record[1] = 'M'; record[2] = 'L'; record[3] = '2';
  record[4] = 1;
  record[5] = settings.wpm;
  record[6] = static_cast<uint8_t>(std::strlen(settings.message));
  std::memcpy(record.data() + 8, settings.message, record[6]);
  const uint32_t crc = checksum(record);
  for (size_t i = 0; i < 4; ++i) record[128 + i] = static_cast<uint8_t>(crc >> (i * 8));
  return record;
}

bool decodeSettings(const SettingsRecord& record, Settings& settings) {
  if (record[0] != 'X' || record[1] != 'M' || record[2] != 'L' || record[3] != '2'
      || record[4] != 1 || record[7] != 0 || record[6] == 0 || record[6] > MAX_MESSAGE_LENGTH) return false;
  uint32_t storedCrc = 0;
  for (size_t i = 0; i < 4; ++i) storedCrc |= static_cast<uint32_t>(record[128 + i]) << (i * 8);
  if (storedCrc != checksum(record)) return false;
  Settings candidate;
  const std::string_view message(reinterpret_cast<const char*>(record.data() + 8), record[6]);
  if (normalizeSettings(message, record[5], candidate) != SettingsError::None
      || message != candidate.message) return false;
  settings = candidate;
  return true;
}

bool Configuration::begin() {
  settings = Settings{};
  SettingsRecord record{};
  return storage.read(record) && decodeSettings(record, settings);
}

SaveResult Configuration::save(const Settings& candidate) {
  if (!isCanonical(candidate)) return SaveResult::Invalid;
  if (sameSettings(settings, candidate)) return SaveResult::Unchanged;
  if (!storage.write(encodeSettings(candidate))) return SaveResult::StorageError;
  settings = candidate;
  return SaveResult::Saved;
}
