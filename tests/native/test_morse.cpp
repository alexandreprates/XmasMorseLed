#include "MorseTransmitter.h"
#include "MorseTable.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct Edge { uint32_t time; bool high; };
struct Trace {
  uint32_t now = 0;
  std::vector<Edge> edges;
  static void write(bool high, void* context) {
    auto& self = *static_cast<Trace*>(context);
    self.edges.push_back({self.now, high});
  }
};
Settings make(const char* text, int wpm = 25) {
  Settings result;
  assert(normalizeSettings(text, wpm, result) == SettingsError::None);
  return result;
}
void tick(MorseTransmitter& tx, Trace& trace, uint32_t time) {
  trace.now = time;
  tx.update(time);
}
int main() {
  const char* characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?!-/():;=+@";
  const char* patterns[] = {".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..","-----",".----","..---","...--","....-",".....","-....","--...","---..","----.",".-.-.-","--..--","..--..","-.-.--","-....-","-..-.","-.--.","-.--.-","---...","-.-.-.","-...-",".-.-.",".--.-."};
  for (size_t i = 0; characters[i]; ++i) {
    const auto* pattern = MorseTable::findPattern(characters[i]);
    assert(pattern);
    std::string decoded;
    for (uint8_t j = 0; j < pattern->length; ++j) decoded += MorseTable::isDash(*pattern, j) ? '-' : '.';
    assert(decoded == patterns[i]);
    assert(!MorseTable::isDash(*pattern, pattern->length));
  }
  assert(!MorseTable::findPattern('#'));
  assert(MorseTable::findPattern('s') == MorseTable::findPattern('S'));
  assert(MorseTable::findPattern(' ')->length == 0);
  Trace trace;
  MorseTransmitter tx;
  assert(!tx.configure(make("E"), 0));
  tx.begin(Trace::write, &trace);
  assert(!tx.isReady() && !trace.edges.back().high);
  assert(tx.configure(make("ET E"), 0));
  // E on 336..384, T on 528..672, word gap 336, E on 1008..1056.
  const std::vector<Edge> expected = {{0,false},{336,true},{384,false},{528,true},{672,false},{1008,true},{1056,false},{1728,true}};
  for (uint32_t t = 0; t <= 1728; ++t) tick(tx, trace, t);
  assert(trace.edges.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    assert(trace.edges[i].time == expected[i].time && trace.edges[i].high == expected[i].high);
  }
  // Restart during a pulse uses the new speed and a full seven-unit dark gap.
  trace.now = 1730;
  assert(tx.configure(make("I", 40), trace.now));
  assert(!trace.edges.back().high);
  tick(tx, trace, 1939); assert(!trace.edges.back().high);
  tick(tx, trace, 1940); assert(trace.edges.back().high);
  tick(tx, trace, 1970); assert(!trace.edges.back().high);
  tick(tx, trace, 1999); assert(!trace.edges.back().high);
  tick(tx, trace, 2000); assert(trace.edges.back().high);
  // A delayed scheduler must not compress or emit a burst of missed pulses.
  size_t before = trace.edges.size();
  tick(tx, trace, 10000); assert(trace.edges.size() == before + 1 && !trace.edges.back().high);
  tick(tx, trace, 10000); assert(trace.edges.size() == before + 1);
  // Wraparound and minimum speed.
  const uint32_t start = UINT32_MAX - 100;
  trace.now = start;
  assert(tx.configure(make("T", 5), start));
  tick(tx, trace, start + 1679U); assert(!trace.edges.back().high);
  tick(tx, trace, start + 1680U); assert(trace.edges.back().high);
  tick(tx, trace, start + 2400U); assert(!trace.edges.back().high);
  tick(tx, trace, start + 5760U); assert(trace.edges.back().high);
  // Invalid settings interrupt an active pulse and stop the output.

  Settings invalid = make("E"); invalid.wpm = 0;
  assert(!tx.configure(invalid, 0) && !tx.isReady());
  assert(!trace.edges.back().high);
  before = trace.edges.size(); tick(tx, trace, 100000); assert(trace.edges.size() == before);
  invalid = make("E"); invalid.message[0] = '#';
  assert(!tx.configure(invalid, 0));
  Settings normalized;
  assert(normalizeSettings("  feliz   natal!  ", 25, normalized) == SettingsError::None);
  assert(std::string(normalized.message) == "FELIZ NATAL!" && isCanonical(normalized));
  assert(normalizeSettings(std::string(120, 'E'), 5, normalized) == SettingsError::None);
  assert(normalizeSettings(std::string(121, 'E'), 25, normalized) == SettingsError::MessageLength);
  assert(normalizeSettings("", 25, normalized) == SettingsError::EmptyMessage);
  assert(normalizeSettings("   ", 25, normalized) == SettingsError::EmptyMessage);
  for (const auto& text : {std::string("coração"), std::string("E#E"), std::string("E\tE"), std::string("E\0E", 3)}) {
    assert(normalizeSettings(text, 25, normalized) == SettingsError::UnsupportedCharacter);
  }
  assert(normalizeSettings("E", 4, normalized) == SettingsError::Speed);
  assert(normalizeSettings("E", 41, normalized) == SettingsError::Speed);
  assert(normalizeSettings("E", 40, normalized) == SettingsError::None);
  normalized.message[0] = 'e'; assert(!isCanonical(normalized));
  for (char& c : normalized.message) c = 'E';
  assert(!isCanonical(normalized));
  std::cout << "PASS Morse patterns, pulse/gap timing, restart, stalled scheduler, rollover and fail-safe output\n";
}
