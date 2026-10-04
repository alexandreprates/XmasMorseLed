// Execute the production worker with a deterministic clock and a one-slot queue.
#include "MorseRuntime.h"
#include <Arduino.h>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

namespace {
struct FakeKernel {
  Settings queued;
  bool occupied = false, allocated = false;
  bool failQueue = false, failTask = false, failOverwrite = false;
  int creates = 0, deletes = 0, tasks = 0, overwrites = 0;
  uint32_t now = 0, stopAt = 0;
  TaskFunction_t worker = nullptr;
  void* context = nullptr;
  std::function<void()> onTick;
  std::vector<std::pair<uint32_t, bool>> edges;
} kernel;
struct StopWorker {};

Settings settingsFor(const char* message, int wpm) {
  Settings settings;
  assert(normalizeSettings(message, wpm, settings) == SettingsError::None);
  return settings;
}

void runUntil(uint32_t stopAt) {
  assert(kernel.worker);
  kernel.stopAt = stopAt;
  try {
    kernel.worker(kernel.context);
    assert(false && "The worker must remain alive");
  } catch (const StopWorker&) {
    assert(kernel.now == stopAt + 1);
  }
}

void testInitializationFailures() {
  kernel = {};
  MorseRuntime runtime;
  Settings initial = settingsFor("T", 40);
  assert(!runtime.isReady() && !runtime.apply(initial));
  Settings invalid = initial; invalid.wpm = 0;
  assert(!runtime.begin(invalid));
  assert(kernel.creates == 0 && kernel.edges.empty());

  kernel.failQueue = true;
  assert(!runtime.begin(initial) && !runtime.isReady());
  assert(kernel.tasks == 0 && kernel.deletes == 0);
  assert(kernel.edges.size() == 1 && !kernel.edges.back().second);

  kernel.failQueue = false; kernel.failTask = true;
  assert(!runtime.begin(initial) && !runtime.isReady());
  assert(kernel.deletes == 1 && !kernel.allocated);
  assert(!runtime.apply(initial));

  kernel.failTask = false;
  assert(runtime.begin(initial) && runtime.isReady());
  const int creates = kernel.creates, tasks = kernel.tasks;
  assert(!runtime.begin(initial));
  assert(kernel.creates == creates && kernel.tasks == tasks);
  runUntil(300);
  const std::vector<std::pair<uint32_t, bool>> expected{{0, false}, {210, true}, {300, false}};
  assert(kernel.edges == expected);
}

void testLatestSettingsInterruptAndCopy() {
  kernel = {};
  MorseRuntime runtime;
  Settings initial = settingsFor("T", 40);
  assert(runtime.begin(initial));
  initial = settingsFor("E", 5); // Queues must copy the caller's value.
  kernel.onTick = [&] {
    if (kernel.now != 220) return;
    assert(kernel.edges.back().second);
    assert(runtime.apply(settingsFor("SOS", 25)));
    Settings latest = settingsFor("E", 5);
    assert(runtime.apply(latest));
    latest = settingsFor("T", 40);
    // apply() only queues work; GPIO changes belong to the worker.
    assert(kernel.edges.back().second);
  };
  runUntil(2140);
  // The in-progress dash stops, intermediate SOS is superseded, then E uses
  // the new speed: seven units (1680 ms) off, one unit (240 ms) on.
  const std::vector<std::pair<uint32_t, bool>> expected{
      {0, false}, {210, true}, {220, false}, {1900, true}, {2140, false}};
  assert(kernel.edges == expected);
}

void testRejectedUpdatesPreservePlayback() {
  kernel = {};
  MorseRuntime runtime;
  assert(runtime.begin(settingsFor("T", 40)));
  kernel.onTick = [&] {
    if (kernel.now != 220) return;
    const int overwrites = kernel.overwrites;
    Settings invalid = settingsFor("E", 5); invalid.message[0] = 'e';
    assert(!runtime.apply(invalid) && kernel.overwrites == overwrites);
    kernel.failOverwrite = true;
    assert(!runtime.apply(settingsFor("E", 5)));
    kernel.failOverwrite = false;
  };
  runUntil(300);
  const std::vector<std::pair<uint32_t, bool>> expected{{0, false}, {210, true}, {300, false}};
  assert(kernel.edges == expected);
}
} // namespace

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t itemSize) {
  ++kernel.creates;
  assert(length == 1 && itemSize == sizeof(Settings) && !kernel.allocated);
  if (kernel.failQueue) return nullptr;
  kernel.allocated = true;
  kernel.occupied = false;
  return &kernel.queued;
}
BaseType_t xQueueOverwrite(QueueHandle_t queue, const void* item) {
  assert(kernel.allocated && queue == &kernel.queued);
  ++kernel.overwrites;
  if (kernel.failOverwrite) return pdFALSE;
  std::memcpy(&kernel.queued, item, sizeof(Settings));
  kernel.occupied = true;
  return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t queue, void* item, TickType_t wait) {
  assert(kernel.allocated && queue == &kernel.queued && wait == 0);
  if (!kernel.occupied) return pdFALSE;
  std::memcpy(item, &kernel.queued, sizeof(Settings));
  kernel.occupied = false;
  return pdTRUE;
}
void vQueueDelete(QueueHandle_t queue) {
  assert(kernel.allocated && queue == &kernel.queued);
  ++kernel.deletes;
  kernel.allocated = false;
  kernel.occupied = false;
}
BaseType_t xTaskCreate(TaskFunction_t task, const char*, uint32_t,
                       void* context, UBaseType_t, TaskHandle_t*) {
  ++kernel.tasks;
  if (kernel.failTask) return pdFALSE;
  kernel.worker = task;
  kernel.context = context;
  return pdPASS;
}
void vTaskDelay(TickType_t ticks) {
  assert(ticks > 0);
  kernel.now += ticks;
  if (kernel.now > kernel.stopAt) throw StopWorker{};
  if (kernel.onTick) kernel.onTick();
}
unsigned long millis() { return kernel.now; }
void digitalWrite(uint8_t pin, uint8_t value) {
  assert(pin == MORSE_OUTPUT_PIN && (value == LOW || value == HIGH));
  const bool high = value == HIGH;
  if (kernel.edges.empty() || kernel.edges.back().second != high)
    kernel.edges.emplace_back(kernel.now, high);
}
void pinMode(uint8_t pin, uint8_t mode) {
  assert(pin == MORSE_OUTPUT_PIN && mode == OUTPUT);
  assert(!kernel.edges.empty() && !kernel.edges.back().second);
}

int main() {
  testInitializationFailures();
  testLatestSettingsInterruptAndCopy();
  testRejectedUpdatesPreservePlayback();
  std::cout << "PASS production runtime: startup failures, retry, queue ownership, latest update, pulse interruption and rejected updates\n";
}
