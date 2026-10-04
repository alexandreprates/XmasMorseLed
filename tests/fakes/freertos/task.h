#pragma once
#include "FreeRTOS.h"
BaseType_t xTaskCreate(TaskFunction_t task, const char* name, uint32_t stackDepth,
                       void* context, UBaseType_t priority, TaskHandle_t* handle);
void vTaskDelay(TickType_t ticks);
