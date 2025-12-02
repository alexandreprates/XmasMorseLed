#include <Arduino.h>
#include "MorseTransmitter.h"

const uint8_t LED_PIN = 0;        // PB0 on ATtiny85
const uint8_t WPM = 25;           // Words per minute (international standard)
const char* const MESSAGE = "FELIZ NATAL!";

MorseTransmitter transmitter;

void setup() {
  transmitter.begin(WPM, LED_PIN);
}

void loop() {
  transmitter.transmitMessage(MESSAGE);
}
