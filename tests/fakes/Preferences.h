#pragma once
#include <cstddef>

// Implemented by test_preferences_storage.cpp; only used by host tests.
class Preferences {
public:
  bool begin(const char* name, bool readOnly = false);
  size_t getBytesLength(const char* key);
  size_t getBytes(const char* key, void* buffer, size_t size);
  size_t putBytes(const char* key, const void* buffer, size_t size);
  void end();
};
