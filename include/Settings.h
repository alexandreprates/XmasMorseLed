#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstddef>
#include <cstdint>
#include <string_view>

constexpr size_t MAX_MESSAGE_LENGTH = 120;
constexpr uint8_t MIN_WPM = 5;
constexpr uint8_t MAX_WPM = 40;

struct Settings {
  char message[MAX_MESSAGE_LENGTH + 1] = "FELIZ NATAL!";
  uint8_t wpm = 25;
};

enum class SettingsError { None, MessageLength, EmptyMessage, UnsupportedCharacter, Speed };

// Returns a canonical copy; output remains untouched when validation fails.
SettingsError normalizeSettings(std::string_view message, int wpm, Settings& output);
bool isCanonical(const Settings& settings);
bool sameSettings(const Settings& left, const Settings& right);

#endif
