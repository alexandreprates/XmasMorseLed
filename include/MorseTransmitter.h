#ifndef MORSE_TRANSMITTER_H
#define MORSE_TRANSMITTER_H

#include "Settings.h"

class MorseTransmitter {
public:
  using Output = void (*)(bool high, void* context);
  void begin(Output output, void* context = nullptr);
  bool configure(const Settings& settings, uint32_t nowMs);
  void update(uint32_t nowMs);
  bool isReady() const { return active; }

private:
  void setOutput(bool high);
  void startPulse(uint32_t nowMs);
  Output output = nullptr;
  void* context = nullptr;
  Settings settings{};
  size_t character = 0;
  uint8_t symbol = 0;
  uint32_t startedAt = 0;
  uint32_t duration = 0;
  uint16_t unit = 0;
  bool active = false;
  bool high = false;
};

#endif
