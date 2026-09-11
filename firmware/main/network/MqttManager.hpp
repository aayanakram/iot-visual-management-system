#pragma once

#include <cstdint>
#include <atomic>
#include <string>

#include "mqtt_client.h"

#include "EventQueue.hpp"

// Manages MQTT communication between the ESP32-S3 station and the external backend.
//
// Responsibilities include:
// - broker connection
// - publishing station events
// - subscribing to backend commands
// - reconnect handling
// - converting MQTT connection state into firmware events
class MqttManager
{
public:
    MqttManager(
        const char* brokerUri,
        const char* deviceId,
        EventQueue& eventQueue
    );

    ~MqttManager();

    // initialize the ESP-IDF MQTT client
    bool initialize();

    // start the MQTT client and begin connecting to the broker
    bool start();

    // stop the MQTT client
    void stop();

    // publish a message to a topic
    // returns true if ESP-IDF accepted the message for transmission
    bool publish(
        const std::string& topic,
        const std::string& payload,
        int qos = 1,
        bool retain = false
    );

    // subscribe to a topic used for backend-to-device communication
    bool subscribe(
        const std::string& topic,
        int qos = 1
    );

    // returns true while connected to the MQTT broker
    bool isConnected() const;

private:
    const char* brokerUri_;
    const char* deviceId_;

    EventQueue& eventQueue_;

    esp_mqtt_client_handle_t client_ = nullptr;

    bool initialized_ = false;
    std::atomic<bool> connected_{false};

    // static callback registered with ESP-IDF
    static void eventHandler(
        void* handlerArgs,
        esp_event_base_t base,
        std::int32_t eventId,
        void* eventData
    );

    // process MQTT events after the static callback redirects them to this MqttManager instance.
    void handleEvent(esp_mqtt_event_handle_t event);

    // publish MQTT connectivity state into the firmware event queue
    void publishConnectionEvent(EventType type);

    // convert an incoming MQTT command into an internal firmware event
    void handleIncomingMessage(esp_mqtt_event_handle_t event);

    // build the station-specific command topic
    std::string commandTopic() const;
};
