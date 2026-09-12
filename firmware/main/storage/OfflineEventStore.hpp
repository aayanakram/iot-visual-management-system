#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "Event.hpp"

// Stores outgoing station events while MQTT cannot accept them.
//
// The current prototype uses a bounded in-memory queue. A future physical deployment can extend this class to persist events in NVS or flash so that
// queued events survive power loss. The current RAM queue does not survive reboot.
//
// Physical inputs report absolute state rather than incremental occurrences, so
// only the newest buffered value per source is worth replaying. push() therefore
// coalesces: a new event from a physical source replaces any older buffered event
// from that same source. This keeps the backlog bounded by the number of sources
// instead of by the duration of the outage, so replay ends at the station's
// current value rather than at a stale one.
class OfflineEventStore
{
public:
    // Logical source used by system events (heartbeat, fault, sync request).
    // These describe discrete occurrences rather than absolute state, so they
    // are never coalesced.
    static constexpr std::uint32_t SYSTEM_SOURCE_ID = 0;

    explicit OfflineEventStore(std::size_t capacity);
    ~OfflineEventStore();

    // Prevent copying because this class owns a FreeRTOS mutex.
    OfflineEventStore(const OfflineEventStore&) = delete;
    OfflineEventStore& operator=(const OfflineEventStore&) = delete;

    // Store an event for later transmission.
    //
    // An event from a physical source replaces any older buffered event carrying
    // the same source ID, and the surviving event moves to the back so replay
    // still delivers ascending sequence IDs. System-source events are appended
    // without coalescing.
    //
    // Returns false if the store is unavailable, or has reached capacity with no
    // older event from this source to replace.
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

    // Number of buffered events that have been superseded by a newer value from
    // the same source. Reported in logs so a long outage shows how much input
    // motion was collapsed rather than silently discarded.
    std::size_t coalescedCount();

private:
    std::size_t capacity_;

    std::deque<Event> events_;

    // Count of events replaced by a newer value from the same source.
    std::size_t coalescedCount_ = 0;

    // Protect the queue because network and application tasks may access it from different FreeRTOS execution contexts.
    SemaphoreHandle_t mutex_ = nullptr;

    bool lock();
    void unlock();
};
