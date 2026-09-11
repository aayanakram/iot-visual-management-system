#pragma once

#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "AnalogInput.hpp"
#include "DigitalInput.hpp"
#include "RotaryEncoder.hpp"

// Coordinates the station's physical input drivers.
//
// A dedicated FreeRTOS task periodically polls the input drivers.
// Each driver generates Events independently and places them into
// the shared EventQueue when a meaningful state change occurs.
class InputManager
{
public:
    InputManager(
        DigitalInput& buttonInput,
        DigitalInput& toggleInput,
        RotaryEncoder& rotaryEncoder,
        AnalogInput& analogInput
    );

    // Initialize every configured input driver.
    bool initialize();

    // Start the periodic FreeRTOS input polling task.
    bool start();

private:
    DigitalInput& buttonInput_;
    DigitalInput& toggleInput_;
    RotaryEncoder& rotaryEncoder_;
    AnalogInput& analogInput_;

    TaskHandle_t taskHandle_ = nullptr;

    // Polling period balances responsiveness with CPU usage.
    static constexpr std::uint32_t POLL_INTERVAL_MS = 10;

    // FreeRTOS-compatible static task entry point.
    static void taskEntry(void* context);

    // Main polling loop.
    void run();
};