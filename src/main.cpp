#include <Arduino.h>
#include "MorseTransmitter.h"

namespace {
MorseTransmitter transmitter;
void writeOutput(bool high, void*) {
  digitalWrite(MORSE_OUTPUT_PIN, high ? HIGH : LOW);
}
}

void setup() {
  digitalWrite(MORSE_OUTPUT_PIN, LOW);
  pinMode(MORSE_OUTPUT_PIN, OUTPUT);
  transmitter.begin(writeOutput);
  transmitter.configure(Settings{}, millis());
}

void loop() {
  transmitter.update(millis());
  delay(1);
}
