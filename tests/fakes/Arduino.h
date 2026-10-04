#pragma once
#include <cstdint>
constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int OUTPUT = 3;
void digitalWrite(uint8_t pin, uint8_t value);
void pinMode(uint8_t pin, uint8_t mode);
unsigned long millis();
