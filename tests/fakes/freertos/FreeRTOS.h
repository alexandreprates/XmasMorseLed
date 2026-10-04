#pragma once
#include <cstdint>
using QueueHandle_t = void*;
using BaseType_t = int;
using UBaseType_t = unsigned int;
using TickType_t = uint32_t;
using TaskHandle_t = void*;
using TaskFunction_t = void (*)(void*);
constexpr BaseType_t pdPASS = 1;
constexpr BaseType_t pdTRUE = 1;
constexpr BaseType_t pdFALSE = 0;
