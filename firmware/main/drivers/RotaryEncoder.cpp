#include "RotaryEncoder.hpp"

#include "esp_timer.h"
#include <limits>

RotaryEncoder::RotaryEncoder(
    gpio_num_t channelAPin,
    gpio_num_t channelBPin,
    std::uint32_t sourceId,
    EventQueue& eventQueue
)
    : channelAPin_(channelAPin),
      channelBPin_(channelBPin),
      sourceId_(sourceId),
      eventQueue_(eventQueue)
{
}

bool RotaryEncoder::initialize()
{
    gpio_config_t config = {};

    // configure both encoder channels as inputs with internal pull-ups
    config.pin_bit_mask =
        (1ULL << static_cast<unsigned>(channelAPin_)) |
        (1ULL << static_cast<unsigned>(channelBPin_));

    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&config) != ESP_OK)
    {
        return false;
    }

    // capture the initial quadrature state before polling begins
    previousState_ = readState();

    return true;
}

void RotaryEncoder::update()
{
    const std::uint8_t currentState = readState();

    // no movement occurred if both encoder channels remain unchanged
    if (currentState == previousState_)
    {
        return;
    }

    /*
     * Quadrature transition lookup table.
     *
     * Each index is formed from:
     *     previousState << 2 | currentState
     *
     * Valid transitions produce +1 or -1.
     * Invalid transitions produce 0 and are ignored.
     */
    static constexpr std::int8_t transitionTable[16] =
    {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    const std::uint8_t transitionIndex =
        static_cast<std::uint8_t>((previousState_ << 2) | currentState);

    const std::int8_t movement = transitionTable[transitionIndex];

    previousState_ = currentState;

    // ignore impossible or noisy state transitions
    if (movement == 0)
    {
        return;
    }

    if ((movement > 0 && count_ == std::numeric_limits<std::int32_t>::max()) ||
        (movement < 0 && count_ == std::numeric_limits<std::int32_t>::min()))
    {
        return;
    }
    count_ += movement;
    publishCount();
}

std::int32_t RotaryEncoder::count() const
{
    return count_;
}

std::uint8_t RotaryEncoder::readState() const
{
    const std::uint8_t channelA =
        gpio_get_level(channelAPin_) ? 1 : 0;

    const std::uint8_t channelB =
        gpio_get_level(channelBPin_) ? 1 : 0;

    // store Channel A as bit 1 and Channel B as bit 0
    return static_cast<std::uint8_t>((channelA << 1) | channelB);
}

void RotaryEncoder::publishCount()
{
    Event event;

    event.type = EventType::EncoderChanged;
    event.sourceId = sourceId_;
    event.value = count_;

    // convert ESP-IDF's microsecond timer into milliseconds
    event.timestampMs =
        static_cast<std::uint64_t>(esp_timer_get_time() / 1000);

    // sequence IDs will be assigned centrally by the application layer
    event.sequenceId = 0;

    eventQueue_.send(event);
}
