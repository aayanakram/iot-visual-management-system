#pragma once

#include <cstddef>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "Event.hpp"

// small C++ wrapper around a FreeRTOS queue used to move event objects between firmware tasks without tightly coupling the modules together
class EventQueue
{
public:
    explicit EventQueue(std::size_t capacity);
    ~EventQueue();

    // prevent accidental copying because the class owns a FreeRTOS queue handle
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    // add an event to the queue
    // returns true when the event was successfully queued
    bool send(const Event& event, TickType_t timeout = 0);

    // receive the next event from the queue
    // by default, the calling task waits until an event becomes available
    bool receive(Event& event, TickType_t timeout = portMAX_DELAY);

    // allows initialization code to verify that the queue was created
    bool isValid() const;

private:
    QueueHandle_t queueHandle_ = nullptr;
};