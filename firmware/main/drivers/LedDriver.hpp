#pragma once

#include "driver/gpio.h"

// simple digital LED/status-output driver
// the class abstracts the GPIO details so higher-level firmware only needs to set the desired logical output state.
class LedDriver
{
public:
    LedDriver(
        gpio_num_t pin,
        bool activeHigh = true
    );

    // configure the GPIO as an output
    bool initialize();

    // set the logical LED/output state
    void set(bool on);

    // toggle the current logical output state
    void toggle();

    // return the current logical state
    bool state() const;

private:
    gpio_num_t pin_;
    bool activeHigh_;
    bool state_ = false;

    // apply the logical state to the physical GPIO level
    void writeState();
};