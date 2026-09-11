#include "MqttManager.hpp"

#include <string>

#include "MessageSerializer.hpp"
#include "DeviceConfig.hpp"

#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "MqttManager";

MqttManager::MqttManager(
    const char* brokerUri,
    const char* deviceId,
    EventQueue& eventQueue
)
    : brokerUri_(brokerUri),
      deviceId_(deviceId),
      eventQueue_(eventQueue)
{
}

MqttManager::~MqttManager()
{
    if (client_ != nullptr)
    {
        esp_mqtt_client_stop(client_);
        esp_mqtt_client_destroy(client_);
        client_ = nullptr;
    }
}

bool MqttManager::initialize()
{
    if (initialized_)
    {
        return true;
    }

    esp_mqtt_client_config_t mqttConfig = {};

    // Broker URI may later use mqtts:// when TLS is configured.
    mqttConfig.broker.address.uri = brokerUri_;
    mqttConfig.credentials.client_id = deviceId_;
    mqttConfig.network.reconnect_timeout_ms = 5000;
    mqttConfig.outbox.limit = 16384;

    client_ = esp_mqtt_client_init(&mqttConfig);

    if (client_ == nullptr)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return false;
    }

    /*
     * ESP-MQTT expects its strongly typed event ID enum.
     * Cast ESP_EVENT_ANY_ID explicitly for C++ compilation.
     */
    if (esp_mqtt_client_register_event(
            client_,
            static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
            &MqttManager::eventHandler,
            this) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");

        esp_mqtt_client_destroy(client_);
        client_ = nullptr;

        return false;
    }

    initialized_ = true;

    ESP_LOGI(TAG, "MQTT client initialized");

    return true;
}

bool MqttManager::start()
{
    if (!initialized_ || client_ == nullptr)
    {
        return false;
    }

    const esp_err_t result = esp_mqtt_client_start(client_);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return false;
    }

    ESP_LOGI(TAG, "MQTT client started");

    return true;
}

void MqttManager::stop()
{
    if (client_ == nullptr)
    {
        return;
    }

    esp_mqtt_client_stop(client_);
    connected_ = false;

    ESP_LOGI(TAG, "MQTT client stopped");
}

bool MqttManager::publish(
    const std::string& topic,
    const std::string& payload,
    int qos,
    bool retain
)
{
    if (!connected_ || client_ == nullptr)
    {
        return false;
    }

    const int messageId = esp_mqtt_client_enqueue(
        client_,
        topic.c_str(),
        payload.c_str(),
        static_cast<int>(payload.length()),
        qos,
        retain ? 1 : 0,
        true
    );

    if (messageId < 0)
    {
        ESP_LOGW(TAG, "Failed to publish MQTT message");
        return false;
    }

    ESP_LOGI(
        TAG,
        "Queued MQTT message to %s (message ID: %d)",
        topic.c_str(),
        messageId
    );

    return true;
}

bool MqttManager::subscribe(
    const std::string& topic,
    int qos
)
{
    if (!connected_ || client_ == nullptr)
    {
        return false;
    }

    const int messageId = esp_mqtt_client_subscribe(
        client_,
        topic.c_str(),
        qos
    );

    if (messageId < 0)
    {
        ESP_LOGW(TAG, "Failed to subscribe to MQTT topic");
        return false;
    }

    ESP_LOGI(
        TAG,
        "Requested subscription to %s (message ID: %d)",
        topic.c_str(),
        messageId
    );

    return true;
}

bool MqttManager::isConnected() const
{
    return connected_;
}

void MqttManager::eventHandler(
    void* handlerArgs,
    esp_event_base_t base,
    std::int32_t eventId,
    void* eventData
)
{
    (void)base;
    (void)eventId;

    auto* manager = static_cast<MqttManager*>(handlerArgs);

    if (manager == nullptr || eventData == nullptr)
    {
        return;
    }

    auto* event =
        static_cast<esp_mqtt_event_handle_t>(eventData);

    manager->handleEvent(event);
}

void MqttManager::handleEvent(
    esp_mqtt_event_handle_t event
)
{
    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            connected_ = true;

            ESP_LOGI(TAG, "Connected to MQTT broker");

            publishConnectionEvent(EventType::MqttConnected);

            // Restore command subscription after every reconnection.
            subscribe(commandTopic(), 1);

            break;
        }

        case MQTT_EVENT_DISCONNECTED:
        {
            connected_ = false;

            ESP_LOGW(TAG, "Disconnected from MQTT broker");

            publishConnectionEvent(EventType::MqttDisconnected);

            // ESP-MQTT automatically attempts reconnection by default.
            break;
        }

        case MQTT_EVENT_DATA:
        {
            handleIncomingMessage(event);
            break;
        }

        case MQTT_EVENT_ERROR:
        {
            ESP_LOGE(TAG, "MQTT transport error");
            break;
        }

        default:
        {
            break;
        }
    }
}

void MqttManager::publishConnectionEvent(
    EventType type
)
{
    Event event;

    event.type = type;
    event.sourceId = 0;
    event.value = connected_ ? 1 : 0;

    event.timestampMs =
        static_cast<std::uint64_t>(
            esp_timer_get_time() / 1000
        );

    event.sequenceId = 0;

    eventQueue_.send(event);
}

void MqttManager::handleIncomingMessage(
    esp_mqtt_event_handle_t event
)
{
    // Commands are small, complete, non-retained messages on this station's topic.
    // Reject fragmented payloads instead of executing a valid JSON prefix.
    if (event->data == nullptr || event->data_len <= 0 ||
        event->data_len > 512 || event->current_data_offset != 0 ||
        event->total_data_len != event->data_len || event->retain ||
        event->topic == nullptr || event->topic_len <= 0 ||
        std::string(event->topic, event->topic_len) != commandTopic())
    {
        return;
    }

    /*
     * MQTT data is not guaranteed to be null terminated,
     * so construct the string using its explicit length.
     */
    const std::string payload(
        event->data,
        static_cast<std::size_t>(event->data_len)
    );

    ESP_LOGI(
        TAG,
        "Received MQTT command: %s",
        payload.c_str()
    );

    Event commandEvent;

    // Convert backend JSON into a normal internal firmware event.
    if (!MessageSerializer::parseCommand(
            payload,
            commandEvent))
    {
        ESP_LOGW(
            TAG,
            "Ignoring invalid or unsupported MQTT command"
        );

        return;
    }

    if (!eventQueue_.send(commandEvent))
    {
        ESP_LOGW(
            TAG,
            "Failed to queue MQTT command event"
        );
    }
}

std::string MqttManager::commandTopic() const
{
    return std::string(config::MQTT_BASE_TOPIC) + "/" +
           deviceId_ +
           "/commands";
}
