#pragma once

#include <cstdint>

#include "driver/gpio.h"

#include "EventQueue.hpp"

// represents a quadrature rotary encoder
// the driver tracks Channel A and Channel B state changes and emits EncoderChanged events containing the updated encoder count.
class RotaryEncoder
{
public:
    RotaryEncoder(
        gpio_num_t channelAPin,
        gpio_num_t channelBPin,
        std::uint32_t sourceId,
        EventQueue& eventQueue
    );

    // configure both encoder channels as digital inputs
    bool initialize();

    // poll the encoder and update the count when a valid quadrature transition is detected.
    void update();

    // current accumulated encoder count
    std::int32_t count() const;

private:
    gpio_num_t channelAPin_;
    gpio_num_t channelBPin_;

    std::uint32_t sourceId_;
    EventQueue& eventQueue_;

    std::int32_t count_ = 0;

    // previous two-bit encoder state: [A B]
    std::uint8_t previousState_ = 0;

    // read the current two-bit quadrature state
    std::uint8_t readState() const;

    // publish the updated encoder count
    void publishCount();
};