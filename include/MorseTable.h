#ifndef MORSE_TABLE_H
#define MORSE_TABLE_H

#include <cstdint>

namespace MorseTable {
// LSB-first symbols: zero is a dot, one is a dash; space has length zero.
struct MorsePattern {
  char character;
  uint8_t pattern;
  uint8_t length;
};

const MorsePattern* findPattern(char c);
uint8_t getPatternLength(const MorsePattern& pattern);
bool isDash(const MorsePattern& pattern, uint8_t position);
}  // namespace MorseTable

#endif
