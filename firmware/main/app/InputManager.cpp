#include "InputManager.hpp"

#include "esp_log.h"

static const char* TAG = "InputManager";

InputManager::InputManager(
    DigitalInput& buttonInput,
    DigitalInput& toggleInput,
    RotaryEncoder& rotaryEncoder,
    AnalogInput& analogInput
)
    : buttonInput_(buttonInput),
      toggleInput_(toggleInput),
      rotaryEncoder_(rotaryEncoder),
      analogInput_(analogInput)
{
}

bool InputManager::initialize()
{
    if (!buttonInput_.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize button input");
        return false;
    }

    if (!toggleInput_.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize toggle input");
        return false;
    }

    if (!rotaryEncoder_.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize rotary encoder");
        return false;
    }

    if (!analogInput_.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize analog input");
        return false;
    }

    ESP_LOGI(TAG, "Input drivers initialized");

    return true;
}

bool InputManager::start()
{
    if (taskHandle_ != nullptr)
    {
        return true;
    }

    const BaseType_t result = xTaskCreate(
        &InputManager::taskEntry,
        "input_task",
        3072,
        this,
        4,
        &taskHandle_
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create input task");
        taskHandle_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Input polling task started");

    return true;
}

void InputManager::taskEntry(void* context)
{
    auto* manager = static_cast<InputManager*>(context);

    if (manager != nullptr)
    {
        manager->run();
    }

    vTaskDelete(nullptr);
}

void InputManager::run()
{
    const TickType_t pollDelay =
        pdMS_TO_TICKS(POLL_INTERVAL_MS);

    while (true)
    {
        /*
         * Each driver performs its own filtering/debouncing and
         * only generates events when a meaningful change occurs.
         */
        buttonInput_.update();
        toggleInput_.update();
        rotaryEncoder_.update();
        analogInput_.update();

        vTaskDelay(pollDelay);
    }
}