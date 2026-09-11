#pragma once

#include <cstddef>
#include <deque>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "Event.hpp"

// Stores outgoing station events while MQTT cannot accept them.
//
// The current prototype uses a bounded in-memory queue. A future physical deployment can extend this class to persist events in NVS or flash so that
// queued events survive power loss. The current RAM queue does not survive reboot.
class OfflineEventStore
{
public:
    explicit OfflineEventStore(std::size_t capacity);
    ~OfflineEventStore();

    // Prevent copying because this class owns a FreeRTOS mutex.
    OfflineEventStore(const OfflineEventStore&) = delete;
    OfflineEventStore& operator=(const OfflineEventStore&) = delete;

    // Store an event for later transmission.
    //
    // Returns false if the store is unavailable or has reached capacity.
    bool push(const Event& event);

    // Retrieve and remove the oldest stored event.
    //
    // Returns false when no events are waiting.
    bool pop(Event& event);

    // Look at the oldest event without removing it.
    bool peek(Event& event);

    // Remove all buffered events.
    void clear();

    // Number of currently buffered events.
    std::size_t size();

    // Maximum number of events the store can hold.
    std::size_t capacity() const;

    // Returns true when no events are buffered.
    bool empty();

    // Returns true when the buffer has reached its configured capacity.
    bool full();

private:
    std::size_t capacity_;

    std::deque<Event> events_;

    // Protect the queue because network and application tasks may access it from different FreeRTOS execution contexts.
    SemaphoreHandle_t mutex_ = nullptr;

    bool lock();
    void unlock();
};
