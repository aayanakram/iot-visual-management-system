#pragma once

#include <cstdint>

// Defines every event type that can move through the firmware event system.
enum class EventType : std::uint8_t
{
    None = 0,

    // Physical input events.
    ButtonPressed,
    ButtonReleased,
    ToggleChanged,
    EncoderChanged,
    AnalogChanged,

    // Remote/backend events.
    RemoteStateUpdate,
    SetOutput,

    // Connectivity events.
    WifiConnected,
    WifiDisconnected,
    MqttConnected,
    MqttDisconnected,

    // System events.
    SyncRequested,
    FaultDetected,
    Heartbeat
};

// Generic event object passed between firmware modules using FreeRTOS queues.
struct Event
{
    EventType type = EventType::None;

    // Numeric logical source: 1=button, 2=toggle, 3=encoder, 4=analog, 0=system/command.
    std::uint32_t sourceId = 0;

    // Generic integer payload.
    // Examples: 0/1 for digital state, encoder count, or scaled analog value.
    std::int32_t value = 0;

    // Monotonic timestamp in milliseconds when the event was generated.
    std::uint64_t timestampMs = 0;

    // Incrementing identifier used later for ordering and duplicate detection.
    std::uint32_t sequenceId = 0;
};
