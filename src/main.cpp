#include <Arduino.h>
#include "Configuration.h"
#include "ConfigurationServer.h"
#include "MorseRuntime.h"
#include "PreferencesStorage.h"

namespace {
PreferencesStorage storage;
Configuration configuration(storage);
MorseRuntime morse;
ConfigurationServer server(configuration, morse);
}

void setup() {
  digitalWrite(MORSE_OUTPUT_PIN, LOW);
  pinMode(MORSE_OUTPUT_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(configuration.begin() ? "Restored saved settings" : "Using default settings");
  if (!morse.begin(configuration.current())) Serial.println("Morse task initialization failed; output remains off");
  if (!server.begin()) Serial.println("HTTP/Wi-Fi initialization failed; Morse continues with saved settings");
}

void loop() {
  delay(10);
}
