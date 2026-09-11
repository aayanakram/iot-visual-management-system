#pragma once
#include <cstdint>
extern std::int64_t testTimeUs;
inline std::int64_t esp_timer_get_time() { return testTimeUs; }
