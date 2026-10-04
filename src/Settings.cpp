#include "Settings.h"
#include "MorseTable.h"
#include <cstring>

SettingsError normalizeSettings(std::string_view message, int wpm, Settings& output) {
  if (wpm < MIN_WPM || wpm > MAX_WPM) return SettingsError::Speed;
  if (message.size() > MAX_MESSAGE_LENGTH) return SettingsError::MessageLength;
  Settings candidate{};
  std::memset(candidate.message, 0, sizeof(candidate.message));
  candidate.wpm = static_cast<uint8_t>(wpm);
  size_t size = 0;
  bool pendingSpace = false;
  for (char c : message) {
    if (c == ' ') {
      pendingSpace = size > 0;
      continue;
    }
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (!MorseTable::findPattern(c)) return SettingsError::UnsupportedCharacter;
    if (pendingSpace) candidate.message[size++] = ' ';
    candidate.message[size++] = c;
    pendingSpace = false;
  }
  if (size == 0) return SettingsError::EmptyMessage;
  output = candidate;
  return SettingsError::None;
}

bool isCanonical(const Settings& settings) {
  const auto* end = static_cast<const char*>(std::memchr(settings.message, '\0', sizeof(settings.message)));
  if (!end) return false;
  Settings normalized;
  return normalizeSettings(std::string_view(settings.message, end - settings.message), settings.wpm, normalized) == SettingsError::None
      && std::strcmp(settings.message, normalized.message) == 0;
}

bool sameSettings(const Settings& left, const Settings& right) {
  return left.wpm == right.wpm && std::strncmp(left.message, right.message, sizeof(left.message)) == 0;
}
