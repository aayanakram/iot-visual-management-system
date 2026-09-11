#pragma once

#include <cstdint>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "EventQueue.hpp"
#include "MqttManager.hpp"
#include "OfflineEventStore.hpp"
#include "OutputManager.hpp"
#include "StateManager.hpp"

// Coordinates the major firmware subsystems.
//
// The Application task consumes events from the central EventQueue, updates local state and outputs, publishes events through MQTT,
// and buffers outgoing events when MQTT cannot accept them.
class Application
{
public:
    Application(
        EventQueue& eventQueue,
        StateManager& stateManager,
        OutputManager& outputManager,
        MqttManager& mqttManager,
        OfflineEventStore& offlineStore
    );

    // Start the FreeRTOS application-processing task.
    bool start();

private:
    EventQueue& eventQueue_;
    StateManager& stateManager_;
    OutputManager& outputManager_;
    MqttManager& mqttManager_;
    OfflineEventStore& offlineStore_;

    TaskHandle_t taskHandle_ = nullptr;

    // Sequence IDs provide ordering for outgoing station events.
    std::uint32_t nextSequenceId_ = 1;

    // FreeRTOS requires a static task entry point.
    static void taskEntry(void* context);

    // Main application event-processing loop.
    void run();

    // Process one event received from the central event queue.
    void processEvent(Event event);

    // Decide whether an event should be sent to the backend.
    bool shouldPublish(const Event& event) const;

    // Publish immediately when MQTT is available, otherwise buffer locally.
    void publishOrBuffer(const Event& event);

    // Retry buffered events after MQTT connectivity is restored.
    void flushOfflineEvents();

    // Build the MQTT topic used for outgoing station events.
    std::string eventTopic() const;
};
