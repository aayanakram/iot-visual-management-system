#include "EventQueue.hpp"
#include "esp_log.h"

// create a FreeRTOS queue capable of storing the requested number of Event objects
EventQueue::EventQueue(std::size_t capacity)
{
    queueHandle_ = xQueueCreate(
        static_cast<UBaseType_t>(capacity),
        sizeof(Event)
    );
}

EventQueue::~EventQueue()
{
    // release the underlying FreeRTOS queue if it was created successfully
    if (queueHandle_ != nullptr)
    {
        vQueueDelete(queueHandle_);
        queueHandle_ = nullptr;
    }
}

bool EventQueue::send(const Event& event, TickType_t timeout)
{
    if (queueHandle_ == nullptr)
    {
        return false;
    }

    const bool sent = xQueueSend(queueHandle_, &event, timeout) == pdPASS;
    if (!sent)
    {
        ESP_LOGW("EventQueue", "Queue full; event type %u dropped",
                 static_cast<unsigned>(event.type));
    }
    return sent;
}

bool EventQueue::receive(Event& event, TickType_t timeout)
{
    if (queueHandle_ == nullptr)
    {
        return false;
    }

    return xQueueReceive(queueHandle_, &event, timeout) == pdPASS;
}

bool EventQueue::isValid() const
{
    return queueHandle_ != nullptr;
}
