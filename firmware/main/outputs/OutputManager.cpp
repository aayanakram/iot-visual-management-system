#include "OutputManager.hpp"

OutputManager::OutputManager(LedDriver& statusLed)
    : statusLed_(statusLed)
{
}

bool OutputManager::initialize()
{
    // initialize the underlying output driver
    return statusLed_.initialize();
}

void OutputManager::handleEvent(const Event& event)
{
    switch (event.type)
    {
        case EventType::SetOutput:
        case EventType::RemoteStateUpdate:
            // any non-zero event value is treated as an active output state
            statusLed_.set(event.value != 0);
            break;

        default:
            // events unrelated to physical outputs are ignored here
            break;
    }
}

void OutputManager::setStatusLed(bool on)
{
    statusLed_.set(on);
}