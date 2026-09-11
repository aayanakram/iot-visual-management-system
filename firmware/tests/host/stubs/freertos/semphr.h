#pragma once
#include "FreeRTOS.h"
// Single-threaded host shim, not a test of FreeRTOS mutex scheduling.
using SemaphoreHandle_t = int*;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new int(0); }
inline void vSemaphoreDelete(SemaphoreHandle_t value) { delete value; }
inline int xSemaphoreTake(SemaphoreHandle_t, unsigned) { return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}
