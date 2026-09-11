#include "LedDriver.hpp"

LedDriver::LedDriver(
    gpio_num_t pin,
    bool activeHigh
)
    : pin_(pin),
      activeHigh_(activeHigh)
{
}

bool LedDriver::initialize()
{
    gpio_config_t config = {};

    // configure the selected pin as a push-pull digital output
    config.pin_bit_mask = (1ULL << static_cast<unsigned>(pin_));
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&config) != ESP_OK)
    {
        return false;
    }

    // start with the indicator switched off
    state_ = false;
    writeState();

    return true;
}

void LedDriver::set(bool on)
{
    state_ = on;
    writeState();
}

void LedDriver::toggle()
{
    state_ = !state_;
    writeState();
}

bool LedDriver::state() const
{
    return state_;
}

void LedDriver::writeState()
{
    // convert the logical LED state into the electrical GPIO level
    const int level = activeHigh_
        ? (state_ ? 1 : 0)
        : (state_ ? 0 : 1);

    gpio_set_level(pin_, level);
}