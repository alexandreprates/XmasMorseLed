/**
 * @file MorseTransmitter.cpp
 * @brief Morse code LED transmitter implementation
 */

#include "MorseTransmitter.h"

namespace {
  constexpr uint8_t INVALID_PIN = 255;
}

MorseTransmitter::MorseTransmitter()
  : pin(INVALID_PIN),
    dotDuration(0),
    dashDuration(0),
    symbolSpace(0),
    letterSpace(0),
    wordSpace(0),
    initialized(false) {}

void MorseTransmitter::begin(uint8_t wpm, uint8_t ledPin) {
  if (wpm == 0) {
    return;
  }

  pin = ledPin;
  dotDuration = 1200 / wpm;
  dashDuration = dotDuration * 3;
  symbolSpace = dotDuration;
  letterSpace = dotDuration * 3;
  wordSpace = dotDuration * 7;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  initialized = true;
}

void MorseTransmitter::transmitDot() const {
  digitalWrite(pin, HIGH);
  delay(dotDuration);
  digitalWrite(pin, LOW);
  delay(symbolSpace);
}

void MorseTransmitter::transmitDash() const {
  digitalWrite(pin, HIGH);
  delay(dashDuration);
  digitalWrite(pin, LOW);
  delay(symbolSpace);
}

void MorseTransmitter::transmitLetterSpace() const {
  delay(letterSpace - symbolSpace);
}

void MorseTransmitter::transmitWordSpace() const {
  delay(wordSpace - symbolSpace);
}

void MorseTransmitter::transmitPattern(const MorseTable::MorsePattern& pattern) const {
  uint8_t length = MorseTable::getPatternLength(pattern);

  for (uint8_t i = 0; i < length; i++) {
    if (MorseTable::isDash(pattern, i)) {
      transmitDash();
    } else {
      transmitDot();
    }
  }
}

void MorseTransmitter::sendChar(char c) const {
  const MorseTable::MorsePattern* patternPtr = MorseTable::findPattern(c);
  if (patternPtr == nullptr) {
    return;
  }

  MorseTable::MorsePattern pattern{};
  memcpy_P(&pattern, patternPtr, sizeof(MorseTable::MorsePattern));

  if (MorseTable::getPatternLength(pattern) == 0) {
    return;
  }

  transmitPattern(pattern);
}

void MorseTransmitter::transmitMessage(const char* message) const {
  if (!initialized || message == nullptr) {
    return;
  }

  while (*message) {
    char current = *message;
    char next = *(message + 1);

    if (current == ' ') {
      // Collapse consecutive spaces into a single word gap
      while (*(message + 1) == ' ') {
        message++;
      }
      transmitWordSpace();
    } else {
      sendChar(current);

      if (next != '\0' && next != ' ') {
        transmitLetterSpace();
      }
    }

    message++;
  }

  delay(wordSpace * 2);
}

bool MorseTransmitter::isReady() const {
  return initialized && pin != INVALID_PIN;
}
