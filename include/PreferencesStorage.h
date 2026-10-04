#ifndef PREFERENCES_STORAGE_H
#define PREFERENCES_STORAGE_H

#include "Configuration.h"
#include <Preferences.h>

class PreferencesStorage final : public SettingsStorage {
public:
  bool read(SettingsRecord& record) override {
    Preferences preferences;
    if (!preferences.begin("xmasmorse", true)) return false;
    const bool success = preferences.getBytesLength("settings") == record.size()
        && preferences.getBytes("settings", record.data(), record.size()) == record.size();
    preferences.end();
    return success;
  }
  bool write(const SettingsRecord& record) override {
    Preferences preferences;
    if (!preferences.begin("xmasmorse", false)) return false;
    const bool success = preferences.putBytes("settings", record.data(), record.size()) == record.size();
    preferences.end();
    return success;
  }
};

#endif
