#include "AnalogInput.hpp"

#include <algorithm>
#include <cstdlib>
#include "esp_timer.h"

AnalogInput::AnalogInput(
    adc_unit_t unit,
    adc_channel_t channel,
    std::uint32_t sourceId,
    EventQueue& eventQueue,
    std::uint8_t deadbandPercent
)
    : unit_(unit),
      channel_(channel),
      sourceId_(sourceId),
      eventQueue_(eventQueue),
      deadbandPercent_(deadbandPercent)
{
}

AnalogInput::~AnalogInput()
{
    // release the ADC unit if it was successfully initialized
    if (adcHandle_ != nullptr)
    {
        adc_oneshot_del_unit(adcHandle_);
        adcHandle_ = nullptr;
    }
}

bool AnalogInput::initialize()
{
    // configure a one-shot ADC unit for periodic slider/potentiometer sampling
    adc_oneshot_unit_init_cfg_t unitConfig = {};
    unitConfig.unit_id = unit_;
    unitConfig.ulp_mode = ADC_ULP_MODE_DISABLE;

    if (adc_oneshot_new_unit(&unitConfig, &adcHandle_) != ESP_OK)
    {
        return false;
    }

    adc_oneshot_chan_cfg_t channelConfig = {};

    // use 12-bit conversion and a wide attenuation range suitable for signals spanning most of the ESP32-S3's ADC input range.
    channelConfig.bitwidth = ADC_BITWIDTH_12;
    channelConfig.atten = ADC_ATTEN_DB_12;

    if (adc_oneshot_config_channel(
            adcHandle_,
            channel_,
            &channelConfig) != ESP_OK)
    {
        adc_oneshot_del_unit(adcHandle_);
        adcHandle_ = nullptr;
        return false;
    }

    std::int32_t rawValue = 0;

    // capture an initial value without generating a startup event
    if (!readRaw(rawValue))
    {
        adc_oneshot_del_unit(adcHandle_);
        adcHandle_ = nullptr;
        return false;
    }

    stablePercent_ = rawToPercent(rawValue);
    initialized_ = true;

    return true;
}

void AnalogInput::update()
{
    if (!initialized_)
    {
        return;
    }

    std::int32_t rawValue = 0;

    if (!readRaw(rawValue))
    {
        return;
    }

    const std::int32_t currentPercent = rawToPercent(rawValue);

    // ignore small fluctuations caused by ADC noise or tiny physical movement
    if (!initialEventPending_ &&
        (currentPercent == stablePercent_ ||
         std::abs(currentPercent - stablePercent_) < static_cast<std::int32_t>(deadbandPercent_)))
    {
        return;
    }

    if (publishValue(currentPercent))
    {
        stablePercent_ = currentPercent;
        initialEventPending_ = false;
    }
}

std::int32_t AnalogInput::valuePercent() const
{
    return stablePercent_;
}

bool AnalogInput::readRaw(std::int32_t& rawValue)
{
    if (adcHandle_ == nullptr)
    {
        return false;
    }

    int sample = 0;

    if (adc_oneshot_read(adcHandle_, channel_, &sample) != ESP_OK)
    {
        return false;
    }

    rawValue = static_cast<std::int32_t>(sample);
    return true;
}

std::int32_t AnalogInput::rawToPercent(std::int32_t rawValue) const
{
    // a 12-bit ADC produces values from 0 to 4095
    constexpr std::int32_t ADC_MAX_VALUE = 4095;

    rawValue = std::clamp(
        rawValue,
        static_cast<std::int32_t>(0),
        ADC_MAX_VALUE
    );

    return (rawValue * 100) / ADC_MAX_VALUE;
}

bool AnalogInput::publishValue(std::int32_t percent)
{
    Event event;

    event.type = EventType::AnalogChanged;
    event.sourceId = sourceId_;
    event.value = percent;
    event.sequenceId = 0;

    event.timestampMs = static_cast<std::uint64_t>(esp_timer_get_time() / 1000);

    return eventQueue_.send(event);
}
