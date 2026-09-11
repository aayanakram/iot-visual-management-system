#pragma once

#include <cstdint>
#include <atomic>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "EventQueue.hpp"

// Manages ESP32-S3 Wi-Fi station connectivity
//
// Responsibilities include:
// - Wi-Fi initialization
// - connecting to the configured access point
// - detecting connection loss
// - automatic reconnection
// - publishing connectivity events to the firmware event queue
//
// Wi-Fi credentials are supplied at runtime rather than hardcoded here
class WifiManager
{
public:
    WifiManager(
        const char* ssid,
        const char* password,
        EventQueue& eventQueue
    );

    ~WifiManager();

    // initialize ESP-IDF networking and begin connecting to Wi-Fi
    bool initialize();

    // request a Wi-Fi connection
    bool connect();

    // disconnect from the current Wi-Fi network
    void disconnect();

    // returns true when the device has obtained network connectivity
    bool isConnected() const;

private:
    const char* ssid_;
    const char* password_;

    EventQueue& eventQueue_;

    bool initialized_ = false;
    std::atomic<bool> connected_{false};
    std::atomic<bool> reconnectEnabled_{true};

    // ESP-IDF network interface used for Wi-Fi station mode
    esp_netif_t* netif_ = nullptr;

    // handler registrations are stored so they can be removed cleanly
    esp_event_handler_instance_t wifiEventHandler_ = nullptr;
    esp_event_handler_instance_t ipEventHandler_ = nullptr;

    // static callback passed to the ESP-IDF event system
    static void eventHandler(
        void* handlerArg,
        esp_event_base_t eventBase,
        std::int32_t eventId,
        void* eventData
    );

    // process Wi-Fi-specific events after the static callback redirects them to this WifiManager instance.
    void handleWifiEvent(
        esp_event_base_t eventBase,
        std::int32_t eventId,
        void* eventData
    );

    // publish a connectivity event into the main firmware event queue
    void publishConnectionEvent(EventType type);
};
