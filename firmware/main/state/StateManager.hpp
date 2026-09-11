#pragma once

#include <cstdint>

#include "Event.hpp"

// maintains the current logical state of the visual management station
// the StateManager does not talk directly to hardware or MQTT, it only processes application events and stores the resulting station state.
class StateManager
{
public:
    StateManager() = default;

    // apply an incoming event to the local station state
    void handleEvent(const Event& event);

    // current logical state accessors
    bool buttonState() const;
    bool toggleState() const;
    std::int32_t encoderValue() const;
    std::int32_t analogValue() const;
    bool outputState() const;

private:
    bool buttonState_ = false;
    bool toggleState_ = false;

    std::int32_t encoderValue_ = 0;
    std::int32_t analogValue_ = 0;

    bool outputState_ = false;
};