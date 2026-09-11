#include "MessageSerializer.hpp"

#include <cstring>

#include "cJSON.h"
#include "esp_timer.h"

std::string MessageSerializer::serializeEvent(
    const Event& event,
    const char* deviceId,
    const char* firmwareVersion
)
{
    cJSON* root = cJSON_CreateObject();

    if (root == nullptr)
    {
        return {};
    }

    // A partial JSON object must never be reported as a valid event.
    if (!cJSON_AddStringToObject(root, "device_id", deviceId ? deviceId : "unknown") ||
        !cJSON_AddNumberToObject(root, "sequence_id", event.sequenceId) ||
        !cJSON_AddStringToObject(root, "event_type", eventTypeToString(event.type)) ||
        !cJSON_AddNumberToObject(root, "source_id", event.sourceId) ||
        !cJSON_AddNumberToObject(root, "value", event.value) ||
        !cJSON_AddNumberToObject(root, "timestamp_ms", static_cast<double>(event.timestampMs)) ||
        !cJSON_AddStringToObject(root, "firmware_version", firmwareVersion ? firmwareVersion : "unknown"))
    {
        cJSON_Delete(root);
        return {};
    }

    // cJSON allocates the rendered string dynamically.
    char* jsonBuffer = cJSON_PrintUnformatted(root);

    std::string result;

    if (jsonBuffer != nullptr)
    {
        result = jsonBuffer;
        cJSON_free(jsonBuffer);
    }

    cJSON_Delete(root);

    return result;
}

bool MessageSerializer::parseCommand(
    const std::string& payload,
    Event& outputEvent
)
{
    if (payload.empty() || payload.size() > 512 ||
        payload.find('\0') != std::string::npos ||
        payload.find("\\u0000") != std::string::npos)
    {
        return false;
    }
    cJSON* root = cJSON_ParseWithOpts(payload.c_str(), nullptr, true);
    if (!cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        return false;
    }
    const cJSON* command = cJSON_GetObjectItemCaseSensitive(root, "command");
    const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "value");
    // Exactly two fields also rejects duplicates and accidentally misrouted metadata.
    if (cJSON_GetArraySize(root) != 2 || !cJSON_IsString(command) ||
        (!cJSON_IsBool(value) &&
         !(cJSON_IsNumber(value) && (value->valuedouble == 0 || value->valuedouble == 1))))
    {
        cJSON_Delete(root);
        return false;
    }
    EventType type;
    if (std::strcmp(command->valuestring, "set_output") == 0)
    {
        type = EventType::SetOutput;
    }
    else if (std::strcmp(command->valuestring, "set_state") == 0)
    {
        type = EventType::RemoteStateUpdate;
    }
    else
    {
        cJSON_Delete(root);
        return false;
    }
    outputEvent = {};
    outputEvent.type = type;
    outputEvent.value = cJSON_IsTrue(value) || (cJSON_IsNumber(value) && value->valuedouble == 1);
    outputEvent.timestampMs = static_cast<std::uint64_t>(esp_timer_get_time() / 1000);
    cJSON_Delete(root);
    return true;
}

const char* MessageSerializer::eventTypeToString(EventType type)
{
    switch (type)
    {
        case EventType::ButtonPressed:
            return "button_pressed";

        case EventType::ButtonReleased:
            return "button_released";

        case EventType::ToggleChanged:
            return "toggle_changed";

        case EventType::EncoderChanged:
            return "encoder_changed";

        case EventType::AnalogChanged:
            return "analog_changed";

        case EventType::RemoteStateUpdate:
            return "remote_state_update";

        case EventType::SetOutput:
            return "set_output";

        case EventType::WifiConnected:
            return "wifi_connected";

        case EventType::WifiDisconnected:
            return "wifi_disconnected";

        case EventType::MqttConnected:
            return "mqtt_connected";

        case EventType::MqttDisconnected:
            return "mqtt_disconnected";

        case EventType::SyncRequested:
            return "sync_requested";

        case EventType::Heartbeat:
            return "heartbeat";

        case EventType::FaultDetected:
            return "fault_detected";

        case EventType::None:
        default:
            return "none";
    }
}
