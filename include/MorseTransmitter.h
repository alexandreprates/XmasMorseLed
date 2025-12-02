/**
 * @file MorseTransmitter.h
 * @brief Simple Morse code LED transmitter
 */

#ifndef MORSE_TRANSMITTER_H
#define MORSE_TRANSMITTER_H

#include <Arduino.h>
#include "MorseTable.h"

class MorseTransmitter {
public:
  MorseTransmitter();

  void begin(uint8_t wpm, uint8_t ledPin);
  void transmitMessage(const char* message) const;
  bool isReady() const;

private:
  uint8_t pin;
  uint16_t dotDuration;
  uint16_t dashDuration;
  uint16_t symbolSpace;
  uint16_t letterSpace;
  uint16_t wordSpace;
  bool initialized;

  void transmitDot() const;
  void transmitDash() const;
  void transmitLetterSpace() const;
  void transmitWordSpace() const;
  void transmitPattern(const MorseTable::MorsePattern& pattern) const;
  void sendChar(char c) const;
};

#endif // MORSE_TRANSMITTER_H
