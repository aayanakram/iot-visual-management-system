#pragma once

#include <cstddef>
#include <cstdint>

// Optional untracked development configuration. Never commit credentials.
#if __has_include("DeviceConfig.local.hpp")
#include "DeviceConfig.local.hpp"
#endif
#ifndef STATION_DEVICE_ID
#define STATION_DEVICE_ID "station_01"
#endif
#ifndef STATION_WIFI_SSID
#define STATION_WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef STATION_WIFI_PASSWORD
#define STATION_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif
#ifndef STATION_MQTT_URI
#define STATION_MQTT_URI "mqtt://YOUR_BROKER_ADDRESS:1883"
#endif

namespace config
{
    // Unique identifier used in MQTT topics and backend routing.
    constexpr const char* DEVICE_ID = STATION_DEVICE_ID;

    // Firmware version reported with outgoing station events.
    constexpr const char* FIRMWARE_VERSION = "0.1.0";

    // -------------------------------------------------------------------------
    // Network configuration
    // -------------------------------------------------------------------------

    /*
     * Development placeholders only.
     *
     * Real deployment credentials should later be provided through
     * provisioning, menuconfig, secure storage, or another protected method.
     */
    constexpr const char* WIFI_SSID = STATION_WIFI_SSID;
    constexpr const char* WIFI_PASSWORD = STATION_WIFI_PASSWORD;

    // Example:
    // mqtt://192.168.1.100:1883
    constexpr const char* MQTT_BROKER_URI =
        STATION_MQTT_URI;

    // MQTT topic root used by this station.
    constexpr const char* MQTT_BASE_TOPIC = "kaizen/stations";

    // -------------------------------------------------------------------------
    // Application configuration
    // -------------------------------------------------------------------------

    // Maximum number of events held while MQTT connectivity is unavailable.
    constexpr std::size_t OFFLINE_EVENT_QUEUE_SIZE = 64;

    // Maximum number of events waiting in the central FreeRTOS event queue.
    constexpr std::size_t EVENT_QUEUE_SIZE = 32;

    // Period between heartbeat messages.
    constexpr std::uint32_t HEARTBEAT_INTERVAL_MS = 30000;

    // Debounce interval for buttons, toggles and digital sensors.
    constexpr std::uint32_t DIGITAL_DEBOUNCE_MS = 30;

    // Minimum analog percentage change required before generating an event.
    constexpr std::uint8_t ANALOG_DEADBAND_PERCENT = 2;

    /*
     * Prototype status-output GPIO.
     *
     * The final pin must be selected to match the specific ESP32-S3
     * development board and physical station wiring.
     */
    constexpr int STATUS_LED_GPIO = 2;
    // -------------------------------------------------------------------------
    // Prototype input GPIO assignments
    // -------------------------------------------------------------------------

    // These pins are placeholders for the proposed ESP32-S3 station wiring.
    // Final assignments should be verified against the selected development board.
    constexpr int BUTTON_GPIO = 4;
    constexpr int TOGGLE_GPIO = 5;

    constexpr int ENCODER_A_GPIO = 6;
    constexpr int ENCODER_B_GPIO = 7;
}
