#pragma once

#include "LedDriver.hpp"
#include "Event.hpp"

// coordinates physical outputs based on application events
// this keeps output-control logic separate from the StateManager and from the low-level GPIO implementation inside LedDriver.
class OutputManager
{
public:
    explicit OutputManager(LedDriver& statusLed);

    // initialize all managed physical outputs
    bool initialize();

    // apply an event that may affect one or more outputs
    void handleEvent(const Event& event);

    // directly set the primary status indicator
    void setStatusLed(bool on);

private:
    LedDriver& statusLed_;
};