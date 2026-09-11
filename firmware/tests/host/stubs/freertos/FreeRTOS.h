#pragma once
#include <cstdint>
using BaseType_t = int;
using UBaseType_t = unsigned;
using TickType_t = unsigned;
constexpr int pdPASS = 1;
constexpr int pdTRUE = 1;
constexpr unsigned portMAX_DELAY = ~0U;
#define pdMS_TO_TICKS(ms) (ms)
