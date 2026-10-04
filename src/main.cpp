#include <Arduino.h>
#include "Configuration.h"
#include "MorseRuntime.h"
#include "PreferencesStorage.h"

namespace {
PreferencesStorage storage;
Configuration configuration(storage);
MorseRuntime morse;
}

void setup() {
  digitalWrite(MORSE_OUTPUT_PIN, LOW);
  pinMode(MORSE_OUTPUT_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(configuration.begin() ? "Restored saved settings" : "Using default settings");
  if (!morse.begin(configuration.current())) Serial.println("Morse task initialization failed; output remains off");
}

void loop() {
  delay(1);
}
