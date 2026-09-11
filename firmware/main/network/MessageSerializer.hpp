#pragma once

#include <string>

#include "Event.hpp"

// Converts firmware events into structured JSON messages and converts simple backend command payloads into internal events.
class MessageSerializer
{
public:
    // Convert a firmware event into a JSON payload suitable for MQTT.
    static std::string serializeEvent(
        const Event& event,
        const char* deviceId,
        const char* firmwareVersion
    );

    // Parse a backend command payload into a firmware event.
    //
    // Returns true if the payload is valid and a supported command was successfully converted.
    static bool parseCommand(
        const std::string& payload,
        Event& outputEvent
    );

private:
    // Convert an EventType into its JSON string representation.
    static const char* eventTypeToString(EventType type);
};