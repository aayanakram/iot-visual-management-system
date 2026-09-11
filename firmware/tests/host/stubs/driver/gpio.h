#pragma once
#include <cstdint>
using gpio_num_t = int;
constexpr int ESP_OK = 0;
constexpr int GPIO_MODE_INPUT = 1, GPIO_MODE_OUTPUT = 2;
constexpr int GPIO_PULLUP_ENABLE = 1, GPIO_PULLUP_DISABLE = 0;
constexpr int GPIO_PULLDOWN_DISABLE = 0, GPIO_INTR_DISABLE = 0;
struct gpio_config_t {
    std::uint64_t pin_bit_mask;
    int mode, pull_up_en, pull_down_en, intr_type;
};
extern int testGpio[50];
inline int gpio_config(const gpio_config_t*) { return ESP_OK; }
inline int gpio_get_level(int pin) { return testGpio[pin]; }
inline int gpio_set_level(int pin, int level) { testGpio[pin] = level; return ESP_OK; }
