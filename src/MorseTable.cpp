/**
 * @file MorseTable.cpp
 * @brief Implementation of ITU Morse code lookup table
 * @details Optimized for microcontroller Flash memory usage with
 *          efficient linear search suitable for small tables.
 * @author Alexandre Prates - ajfprates@gmail.com
 * @date 2025
 */

#include <Arduino.h>
#include "MorseTable.h"

namespace MorseTable {

  const MorsePattern* findPattern(char c) {
    if (c >= 'a' && c <= 'z') {
      c = c - 'a' + 'A';
    }

    for (uint8_t i = 0; i < TABLE_SIZE; i++) {
      MorsePattern pattern;
      memcpy_P(&pattern, &MORSE_TABLE[i], sizeof(MorsePattern));

      if (pattern.character == c) {
        return &MORSE_TABLE[i];
      }
    }

    return nullptr;
  }

  uint8_t getPatternLength(const MorsePattern& pattern) {
    return pattern.length <= 6 ? pattern.length : 6;
  }

  bool isDash(const MorsePattern& pattern, uint8_t position) {
    if (position >= getPatternLength(pattern)) {
      return false;
    }

    uint8_t mask = 1 << position;  // LSB holds the first symbol
    return (pattern.pattern & mask) != 0;
  }

} // namespace MorseTable
