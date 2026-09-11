#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "EventQueue.hpp"

// represents a debounced digital input such as a push button, toggle switch, selector switch, or digital Hall-effect sensor.
class DigitalInput
{
public:
    DigitalInput(
        gpio_num_t pin,
        std::uint32_t sourceId,
        EventType activeEvent,
        EventType inactiveEvent,
        EventQueue& eventQueue,
        std::uint32_t debounceMs = 30
    );

    // configure the GPIO and capture its initial state
    bool initialize();

    // poll the input and generate an event when a debounced state change occurs
    void update();

    // returns the most recently accepted debounced state
    bool state() const;

private:
    gpio_num_t pin_;
    std::uint32_t sourceId_;

    EventType activeEvent_;
    EventType inactiveEvent_;

    EventQueue& eventQueue_;

    std::uint32_t debounceMs_;

    bool lastRawState_ = false;
    bool stableState_ = false;
    bool initialEventPending_ = true;

    std::uint64_t lastChangeTimeMs_ = 0;

    // read the current electrical state of the GPIO
    bool readRawState() const;

    // return the current monotonic time in milliseconds
    std::uint64_t currentTimeMs() const;

    // create and queue the appropriate application event
    bool publishStateChange(bool newState);
};
