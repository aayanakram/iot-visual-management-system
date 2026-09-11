#pragma once

#include <cstdint>

#include "esp_adc/adc_oneshot.h"

#include "EventQueue.hpp"

// represents an analog station input such as a slider or potentiometer
// raw ADC samples are converted into a normalized percentage value
// a deadband prevents small ADC fluctuations from generating unnecessary events
class AnalogInput
{
public:
    AnalogInput(
        adc_unit_t unit,
        adc_channel_t channel,
        std::uint32_t sourceId,
        EventQueue& eventQueue,
        std::uint8_t deadbandPercent = 2
    );

    ~AnalogInput();

    // configure the ADC unit and channel
    bool initialize();

    // read the ADC and generate an event when the value changes by more than the configured deadband.
    void update();

    // returns the most recently accepted value from 0 to 100
    std::int32_t valuePercent() const;

private:
    adc_unit_t unit_;
    adc_channel_t channel_;

    std::uint32_t sourceId_;
    EventQueue& eventQueue_;

    std::uint8_t deadbandPercent_;

    adc_oneshot_unit_handle_t adcHandle_ = nullptr;

    std::int32_t stablePercent_ = 0;
    bool initialized_ = false;
    bool initialEventPending_ = true;

    // read the raw ADC conversion result
    bool readRaw(std::int32_t& rawValue);

    // convert the ADC reading into a 0-100 percentage
    std::int32_t rawToPercent(std::int32_t rawValue) const;

    // queue an AnalogChanged event
    bool publishValue(std::int32_t percent);
};
