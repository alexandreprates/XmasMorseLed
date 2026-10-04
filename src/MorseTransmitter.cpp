#include "MorseTransmitter.h"
#include "MorseTable.h"

void MorseTransmitter::begin(Output newOutput, void* newContext) {
  setOutput(false);
  output = newOutput;
  context = newContext;
  active = false;
  high = false;
  if (output) output(false, context);
}

void MorseTransmitter::setOutput(bool value) {
  if (high != value && output) output(value, context);
  high = value;
}

bool MorseTransmitter::configure(const Settings& incoming, uint32_t nowMs) {
  setOutput(false);
  active = false;
  if (!output || !isCanonical(incoming)) return false;
  settings = incoming;
  unit = 1200 / settings.wpm;
  character = 0;
  symbol = 0;
  startedAt = nowMs;
  duration = 7U * unit;
  active = true;
  return true;
}

void MorseTransmitter::startPulse(uint32_t nowMs) {
  const auto& pattern = *MorseTable::findPattern(settings.message[character]);
  duration = (MorseTable::isDash(pattern, symbol) ? 3U : 1U) * unit;
  startedAt = nowMs;
  setOutput(true);
}

void MorseTransmitter::update(uint32_t nowMs) {
  // Unsigned subtraction works across millis() rollover. Never replay missed
  // edges in a burst: each physical pulse gets its full requested duration.
  if (!active || static_cast<uint32_t>(nowMs - startedAt) < duration) return;
  if (!high) {
    startPulse(nowMs);
    return;
  }

  setOutput(false);
  startedAt = nowMs;
  const auto& pattern = *MorseTable::findPattern(settings.message[character]);
  if (++symbol < pattern.length) {
    duration = unit;
    return;
  }
  symbol = 0;
  ++character;
  if (settings.message[character] == '\0') {
    character = 0;
    duration = 14U * unit;
  } else if (settings.message[character] == ' ') {
    ++character;
    duration = 7U * unit;
  } else {
    duration = 3U * unit;
  }
}
