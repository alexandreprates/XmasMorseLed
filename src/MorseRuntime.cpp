#include "MorseRuntime.h"
#include "MorseTransmitter.h"
#include <Arduino.h>
#include <type_traits>

static_assert(std::is_trivially_copyable<Settings>::value, "FreeRTOS queues require value-copyable settings");

bool MorseRuntime::begin(const Settings& initial) {
  if (ready || !isCanonical(initial)) return false;
  digitalWrite(MORSE_OUTPUT_PIN, LOW);
  pinMode(MORSE_OUTPUT_PIN, OUTPUT);
  queue = xQueueCreate(1, sizeof(Settings));
  if (!queue) return false;
  xQueueOverwrite(queue, &initial);
  if (xTaskCreate(run, "morse", 3072, this, 2, nullptr) != pdPASS) {
    vQueueDelete(queue);
    queue = nullptr;
    return false;
  }
  ready = true;
  return true;
}

bool MorseRuntime::apply(const Settings& settings) {
  return ready && isCanonical(settings) && xQueueOverwrite(queue, &settings) == pdPASS;
}

void MorseRuntime::run(void* context) {
  auto& self = *static_cast<MorseRuntime*>(context);
  MorseTransmitter transmitter;
  transmitter.begin([](bool high, void*) { digitalWrite(MORSE_OUTPUT_PIN, high ? HIGH : LOW); });
  Settings next;
  while (true) {
    if (xQueueReceive(self.queue, &next, 0) == pdTRUE) transmitter.configure(next, millis());
    transmitter.update(millis());
    vTaskDelay(1);
  }
}
