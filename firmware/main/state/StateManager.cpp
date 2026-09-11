#include "StateManager.hpp"

void StateManager::handleEvent(const Event& event)
{
    switch (event.type)
    {
        case EventType::ButtonPressed:
            buttonState_ = true;
            break;

        case EventType::ButtonReleased:
            buttonState_ = false;
            break;

        case EventType::ToggleChanged:
            toggleState_ = (event.value != 0);
            break;

        case EventType::EncoderChanged:
            encoderValue_ = event.value;
            break;

        case EventType::AnalogChanged:
            analogValue_ = event.value;
            break;

        case EventType::SetOutput:
        case EventType::RemoteStateUpdate:
            outputState_ = (event.value != 0);
            break;

        // connectivity and system events are handled elsewhere
        case EventType::WifiConnected:
        case EventType::WifiDisconnected:
        case EventType::MqttConnected:
        case EventType::MqttDisconnected:
        case EventType::SyncRequested:
        case EventType::FaultDetected:
        case EventType::None:
        default:
            break;
    }
}

bool StateManager::buttonState() const
{
    return buttonState_;
}

bool StateManager::toggleState() const
{
    return toggleState_;
}

std::int32_t StateManager::encoderValue() const
{
    return encoderValue_;
}

std::int32_t StateManager::analogValue() const
{
    return analogValue_;
}

bool StateManager::outputState() const
{
    return outputState_;
}