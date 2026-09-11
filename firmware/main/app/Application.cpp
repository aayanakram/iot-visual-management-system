#include "Application.hpp"

#include "DeviceConfig.hpp"
#include "MessageSerializer.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

static const char* TAG = "Application";

Application::Application(
    EventQueue& eventQueue,
    StateManager& stateManager,
    OutputManager& outputManager,
    MqttManager& mqttManager,
    OfflineEventStore& offlineStore
)
    : eventQueue_(eventQueue),
      stateManager_(stateManager),
      outputManager_(outputManager),
      mqttManager_(mqttManager),
      offlineStore_(offlineStore)
{
}

bool Application::start()
{
    if (taskHandle_ != nullptr)
    {
        return true;
    }

    /*
     * Create the central application task.
     *
     * Most higher-level firmware behavior runs here so drivers, networking and outputs remain loosely coupled.
     */
    const BaseType_t result = xTaskCreate(
        &Application::taskEntry,
        "application_task",
        4096,
        this,
        5,
        &taskHandle_
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create application task");
        taskHandle_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Application task started");
    return true;
}

void Application::taskEntry(void* context)
{
    auto* application = static_cast<Application*>(context);

    if (application != nullptr)
    {
        application->run();
    }

    // The application loop should never normally return.
    vTaskDelete(nullptr);
}

void Application::run()
{
    Event event;
    std::uint64_t lastHeartbeatMs = 0;

    while (true)
    {
        if (eventQueue_.receive(event, pdMS_TO_TICKS(1000)))
        {
            processEvent(event);
        }
        // Retry even if a connection notification was dropped or the outbox was full.
        if (!offlineStore_.empty())
        {
            flushOfflineEvents();
        }
        const auto now = static_cast<std::uint64_t>(esp_timer_get_time() / 1000);
        if (now - lastHeartbeatMs >= config::HEARTBEAT_INTERVAL_MS)
        {
            lastHeartbeatMs = now;
            Event heartbeat;
            heartbeat.type = EventType::Heartbeat;
            heartbeat.timestampMs = now;
            heartbeat.value = static_cast<std::int32_t>(esp_get_free_heap_size());
            processEvent(heartbeat);
        }
    }
}

void Application::processEvent(Event event)
{
    /*
     * Connectivity restoration is handled before normal processing so buffered events can be retried as soon as MQTT becomes available.
     */
    if (event.type == EventType::MqttConnected)
    {
        ESP_LOGI(
            TAG,
            "MQTT restored - flushing %u buffered event(s)",
            static_cast<unsigned>(offlineStore_.size())
        );

        flushOfflineEvents();
    }

    // Update the station's logical state.
    stateManager_.handleEvent(event);

    // Apply any event that controls a local physical output.
    outputManager_.handleEvent(event);

    if (!shouldPublish(event))
    {
        return;
    }

    // Assign an ordering identifier before the event leaves the device.
    event.sequenceId = nextSequenceId_++;

    publishOrBuffer(event);
}

bool Application::shouldPublish(const Event& event) const
{
    /*
     * Physical interactions and faults are sent upstream.
     *
     * Backend commands and connectivity events are intentionally excluded to prevent command echo loops and unnecessary MQTT traffic.
     */
    switch (event.type)
    {
        case EventType::ButtonPressed:
        case EventType::ButtonReleased:
        case EventType::ToggleChanged:
        case EventType::EncoderChanged:
        case EventType::AnalogChanged:
        case EventType::FaultDetected:
        case EventType::SyncRequested:
        case EventType::Heartbeat:
            return true;

        default:
            return false;
    }
}

void Application::publishOrBuffer(const Event& event)
{
    // Older events must be accepted before newer events.
    if (!offlineStore_.empty())
    {
        flushOfflineEvents();
    }
    if (offlineStore_.empty() && mqttManager_.isConnected())
    {
        const std::string payload =
            MessageSerializer::serializeEvent(
                event,
                config::DEVICE_ID,
                config::FIRMWARE_VERSION
            );

        if (!payload.empty() &&
            mqttManager_.publish(eventTopic(), payload, 1, false))
        {
            ESP_LOGI(
                TAG,
                "MQTT accepted event sequence %u",
                static_cast<unsigned>(event.sequenceId)
            );

            return;
        }
    }

    // Stale heartbeats must not consume the limited input-event buffer.
    if (event.type == EventType::Heartbeat)
    {
        return;
    }
    // Preserve the event locally if immediate delivery was not possible.
    if (offlineStore_.push(event))
    {
        ESP_LOGW(
            TAG,
            "Buffered event sequence %u (%u waiting)",
            static_cast<unsigned>(event.sequenceId),
            static_cast<unsigned>(offlineStore_.size())
        );
    }
    else
    {
        ESP_LOGE(
            TAG,
            "Offline event buffer full - event sequence %u dropped",
            static_cast<unsigned>(event.sequenceId)
        );
    }
}

void Application::flushOfflineEvents()
{
    if (!mqttManager_.isConnected())
    {
        return;
    }

    Event event;

    /*
     * Peek first and only remove an event after MQTT accepts it.
     * This avoids losing buffered data if connectivity disappears again while the queue is being flushed.
     */
    while (offlineStore_.peek(event))
    {
        const std::string payload =
            MessageSerializer::serializeEvent(
                event,
                config::DEVICE_ID,
                config::FIRMWARE_VERSION
            );

        if (payload.empty())
        {
            ESP_LOGE(
                TAG,
                "Failed to serialize buffered event sequence %u",
                static_cast<unsigned>(event.sequenceId)
            );

            return;
        }

        if (!mqttManager_.publish(
                eventTopic(),
                payload,
                1,
                false))
        {
            ESP_LOGW(
                TAG,
                "Buffered event flush paused - MQTT unavailable"
            );

            return;
        }

        // Remove only after the MQTT client accepts the publication.
        Event sentEvent;
        offlineStore_.pop(sentEvent);

        ESP_LOGI(
            TAG,
            "Flushed buffered event sequence %u",
            static_cast<unsigned>(event.sequenceId)
        );
    }

    ESP_LOGI(TAG, "Offline event buffer handed to MQTT");
}

std::string Application::eventTopic() const
{
    return std::string(config::MQTT_BASE_TOPIC) +
           "/" +
           config::DEVICE_ID +
           "/events";
}
