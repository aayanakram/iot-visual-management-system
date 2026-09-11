idf_component_register(
    SRCS
        "main.cpp"
        "app/Application.cpp"
        "app/InputManager.cpp"
        "events/EventQueue.cpp"
        "state/StateManager.cpp"
        "drivers/DigitalInput.cpp"
        "drivers/AnalogInput.cpp"
        "drivers/RotaryEncoder.cpp"
        "drivers/LedDriver.cpp"
        "outputs/OutputManager.cpp"
        "network/WifiManager.cpp"
        "network/MqttManager.cpp"
        "network/MessageSerializer.cpp"
        "storage/OfflineEventStore.cpp"

    INCLUDE_DIRS
        "."
        "app"
        "config"
        "drivers"
        "events"
        "network"
        "outputs"
        "state"
        "storage"

    REQUIRES
        esp_wifi
        esp_netif
        esp_event
        esp_driver_gpio
        esp_adc
        mqtt
        cjson
        esp_timer
        nvs_flash
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
