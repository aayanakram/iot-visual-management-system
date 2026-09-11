#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "Application.hpp"
#include "InputManager.hpp"

#include "AnalogInput.hpp"
#include "DigitalInput.hpp"
#include "LedDriver.hpp"
#include "RotaryEncoder.hpp"

#include "DeviceConfig.hpp"
#include "EventQueue.hpp"
#include "MqttManager.hpp"
#include "OfflineEventStore.hpp"
#include "OutputManager.hpp"
#include "StateManager.hpp"
#include "WifiManager.hpp"

static const char* TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(
        TAG,
        "Starting IoT Visual Management Station firmware v%s",
        config::FIRMWARE_VERSION
    );

    // ---------------------------------------------------------------------
    // Persistent ESP-IDF storage
    // ---------------------------------------------------------------------

    esp_err_t nvsResult = nvs_flash_init();

    if (nvsResult == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvsResult == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS requires reinitialization");

        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsResult = nvs_flash_init();
    }

    if (nvsResult != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to initialize NVS: %s",
            esp_err_to_name(nvsResult)
        );

        return;
    }

    // ---------------------------------------------------------------------
    // Core event and state infrastructure
    // ---------------------------------------------------------------------

    static EventQueue eventQueue(
        config::EVENT_QUEUE_SIZE
    );

    if (!eventQueue.isValid())
    {
        ESP_LOGE(TAG, "Failed to create central event queue");
        return;
    }

    static StateManager stateManager;

    static OfflineEventStore offlineStore(
        config::OFFLINE_EVENT_QUEUE_SIZE
    );

    // ---------------------------------------------------------------------
    // Physical output layer
    // ---------------------------------------------------------------------

    static LedDriver statusLed(
        static_cast<gpio_num_t>(config::STATUS_LED_GPIO),
        true
    );

    static OutputManager outputManager(
        statusLed
    );

    if (!outputManager.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize output manager");
        return;
    }

    // ---------------------------------------------------------------------
    // Physical input layer
    // ---------------------------------------------------------------------

    static DigitalInput buttonInput(
        static_cast<gpio_num_t>(config::BUTTON_GPIO),
        1,
        EventType::ButtonPressed,
        EventType::ButtonReleased,
        eventQueue,
        config::DIGITAL_DEBOUNCE_MS
    );

    /*
     * Both electrical transitions are represented as ToggleChanged.
     * The event value distinguishes ON (1) from OFF (0).
     */
    static DigitalInput toggleInput(
        static_cast<gpio_num_t>(config::TOGGLE_GPIO),
        2,
        EventType::ToggleChanged,
        EventType::ToggleChanged,
        eventQueue,
        config::DIGITAL_DEBOUNCE_MS
    );

    static RotaryEncoder rotaryEncoder(
        static_cast<gpio_num_t>(config::ENCODER_A_GPIO),
        static_cast<gpio_num_t>(config::ENCODER_B_GPIO),
        3,
        eventQueue
    );

    /*
     * ADC1 Channel 0 is used as the prototype analog input.
     * ESP32-S3 ADC1 channel 0 maps to GPIO1; board wiring is unvalidated.
     */
    static AnalogInput analogInput(
        ADC_UNIT_1,
        ADC_CHANNEL_0,
        4,
        eventQueue,
        config::ANALOG_DEADBAND_PERCENT
    );

    static InputManager inputManager(
        buttonInput,
        toggleInput,
        rotaryEncoder,
        analogInput
    );

    if (!inputManager.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize input manager");
        return;
    }

    // ---------------------------------------------------------------------
    // Network services
    // ---------------------------------------------------------------------

    static WifiManager wifiManager(
        config::WIFI_SSID,
        config::WIFI_PASSWORD,
        eventQueue
    );

    static MqttManager mqttManager(
        config::MQTT_BROKER_URI,
        config::DEVICE_ID,
        eventQueue
    );

    if (!mqttManager.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT manager");
        return;
    }

    // ---------------------------------------------------------------------
    // Application orchestration
    // ---------------------------------------------------------------------

    static Application application(
        eventQueue,
        stateManager,
        outputManager,
        mqttManager,
        offlineStore
    );

    if (!application.start())
    {
        ESP_LOGE(TAG, "Failed to start application task");
        return;
    }

    // Start input polling after the application is ready to consume events.
    if (!inputManager.start())
    {
        ESP_LOGE(TAG, "Failed to start input manager");
        return;
    }

    // ---------------------------------------------------------------------
    // Wi-Fi startup
    // ---------------------------------------------------------------------

    if (!wifiManager.initialize())
    {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi manager");
        return;
    }

    if (!mqttManager.start())
    {
        ESP_LOGE(TAG, "Failed to start MQTT manager");
        return;
    }

    ESP_LOGI(
        TAG,
        "Firmware initialization complete for device %s",
        config::DEVICE_ID
    );
}