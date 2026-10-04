#ifndef MORSE_RUNTIME_H
#define MORSE_RUNTIME_H

#include "Settings.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Static lifetime. The queue copies settings; only the worker owns the engine.
class MorseRuntime {
public:
  bool begin(const Settings& initial);
  bool apply(const Settings& settings);
  bool isReady() const { return ready; }
private:
  static void run(void* context);
  QueueHandle_t queue = nullptr;
  bool ready = false;
};

#endif
