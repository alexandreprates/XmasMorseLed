#include "MorseTable.h"

namespace MorseTable {
namespace {
  const MorsePattern MORSE_TABLE[] = {
    // Letters A-Z (ITU standard patterns)
    {'A', 0b00000010, 2},    // ·-
    {'B', 0b00000001, 4},    // -···
    {'C', 0b00000101, 4},    // -·-·
    {'D', 0b00000001, 3},    // -··
    {'E', 0b00000000, 1},    // ·
    {'F', 0b00000100, 4},    // ··-·
    {'G', 0b00000011, 3},    // --·
    {'H', 0b00000000, 4},    // ····
    {'I', 0b00000000, 2},    // ··
    {'J', 0b00001110, 4},    // ·---
    {'K', 0b00000101, 3},    // -·-
    {'L', 0b00000010, 4},    // ·-··
    {'M', 0b00000011, 2},    // --
    {'N', 0b00000001, 2},    // -·
    {'O', 0b00000111, 3},    // ---
    {'P', 0b00000110, 4},    // ·--·
    {'Q', 0b00001011, 4},    // --·-
    {'R', 0b00000010, 3},    // ·-·
    {'S', 0b00000000, 3},    // ···
    {'T', 0b00000001, 1},    // -
    {'U', 0b00000100, 3},    // ··-
    {'V', 0b00001000, 4},    // ···-
    {'W', 0b00000110, 3},    // ·--
    {'X', 0b00001001, 4},    // -··-
    {'Y', 0b00001101, 4},    // -·--
    {'Z', 0b00000011, 4},    // --··

    // Numbers 0-9
    {'0', 0b00011111, 5},    // -----
    {'1', 0b00011110, 5},    // ·----
    {'2', 0b00011100, 5},    // ··---
    {'3', 0b00011000, 5},    // ···--
    {'4', 0b00010000, 5},    // ····-
    {'5', 0b00000000, 5},    // ·····
    {'6', 0b00000001, 5},    // -····
    {'7', 0b00000011, 5},    // --···
    {'8', 0b00000111, 5},    // ---··
    {'9', 0b00001111, 5},    // ----·

    // Common punctuation
    {'.', 0b00101010, 6},    // ·-·-·-
    {',', 0b00110011, 6},    // --··--
    {'?', 0b00001100, 6},    // ··--··
    {'!', 0b00110101, 6},    // -·-·--
    {'-', 0b00100001, 6},    // -····-
    {'/', 0b00001001, 5},    // -··-·
    {'(', 0b00001101, 5},    // -·--·
    {')', 0b00101101, 6},    // -·--·-
    {':', 0b00000111, 6},    // ---···
    {';', 0b00010101, 6},    // -·-·-·
    {'=', 0b00010001, 5},    // -···-
    {'+', 0b00001010, 5},    // ·-·-·
    {'@', 0b00010110, 6},    // ·--·-·

    // Special character for word spacing
    {' ', 0b00000000, 0}     // Word separator (special case)
  };
}

const MorsePattern* findPattern(char c) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  for (const auto& pattern : MORSE_TABLE) {
    if (pattern.character == c) return &pattern;
  }
  return nullptr;
}

uint8_t getPatternLength(const MorsePattern& pattern) {
  return pattern.length <= 6 ? pattern.length : 6;
}

bool isDash(const MorsePattern& pattern, uint8_t position) {
  return position < getPatternLength(pattern) && (pattern.pattern & (1U << position)) != 0;
}
}  // namespace MorseTable
