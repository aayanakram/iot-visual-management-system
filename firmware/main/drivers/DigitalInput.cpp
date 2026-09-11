#include "DigitalInput.hpp"

#include "esp_timer.h"

DigitalInput::DigitalInput(
    gpio_num_t pin,
    std::uint32_t sourceId,
    EventType activeEvent,
    EventType inactiveEvent,
    EventQueue& eventQueue,
    std::uint32_t debounceMs
)
    : pin_(pin),
      sourceId_(sourceId),
      activeEvent_(activeEvent),
      inactiveEvent_(inactiveEvent),
      eventQueue_(eventQueue),
      debounceMs_(debounceMs)
{
}

bool DigitalInput::initialize()
{
    gpio_config_t config = {};

    // configure the pin as an input with an internal pull-up
    // this matches the active-low wiring proposed in the hardware plan
    config.pin_bit_mask = (1ULL << static_cast<unsigned>(pin_));
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&config) != ESP_OK)
    {
        return false;
    }

    lastRawState_ = readRawState();
    stableState_ = lastRawState_;
    lastChangeTimeMs_ = currentTimeMs();

    return true;
}

void DigitalInput::update()
{
    const bool rawState = readRawState();
    const std::uint64_t now = currentTimeMs();

    // a raw transition restarts the debounce timer
    if (rawState != lastRawState_)
    {
        lastRawState_ = rawState;
        lastChangeTimeMs_ = now;
    }

    // only accept a new state after it has remained stable long enough
    if ((initialEventPending_ || rawState != stableState_) &&
        ((now - lastChangeTimeMs_) >= debounceMs_))
    {
        if (publishStateChange(rawState))
        {
            stableState_ = rawState;
            initialEventPending_ = false;
        }
    }
}

bool DigitalInput::state() const
{
    return stableState_;
}

bool DigitalInput::readRawState() const
{
    // inputs are wired active-low, so LOW means the control is active
    return gpio_get_level(pin_) == 0;
}

std::uint64_t DigitalInput::currentTimeMs() const
{
    // esp_timer_get_time() returns microseconds since boot
    return static_cast<std::uint64_t>(esp_timer_get_time() / 1000);
}

bool DigitalInput::publishStateChange(bool newState)
{
    Event event;

    event.type = newState ? activeEvent_ : inactiveEvent_;
    event.sourceId = sourceId_;
    event.value = newState ? 1 : 0;
    event.timestampMs = currentTimeMs();

    // Sequence IDs are assigned centrally by Application.
    event.sequenceId = 0;

    return eventQueue_.send(event);
}
