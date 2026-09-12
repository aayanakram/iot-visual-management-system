#include "OfflineEventStore.hpp"

OfflineEventStore::OfflineEventStore(std::size_t capacity)
    : capacity_(capacity)
{
    // A mutex protects the buffer when accessed by multiple FreeRTOS tasks.
    mutex_ = xSemaphoreCreateMutex();
}

OfflineEventStore::~OfflineEventStore()
{
    if (mutex_ != nullptr)
    {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

bool OfflineEventStore::push(const Event& event)
{
    if (!lock())
    {
        return false;
    }

    /*
     * Physical inputs carry absolute state, so an older buffered value from the
     * same source is already obsolete. Replacing it keeps the backlog bounded by
     * the number of sources rather than by the length of the outage, which is
     * what lets replay finish at the station's current value.
     *
     * System-source events describe discrete occurrences instead of state, so
     * they are left alone.
     */
    if (event.sourceId != SYSTEM_SOURCE_ID)
    {
        for (auto it = events_.begin(); it != events_.end(); ++it)
        {
            if (it->sourceId == event.sourceId)
            {
                events_.erase(it);
                ++coalescedCount_;
                break;
            }
        }
    }

    // Keep the buffer bounded so disconnected operation cannot consume memory indefinitely.
    if (events_.size() >= capacity_)
    {
        unlock();
        return false;
    }

    // The newest value goes to the back so replay still ascends by sequence ID.
    events_.push_back(event);

    unlock();
    return true;
}

bool OfflineEventStore::pop(Event& event)
{
    if (!lock())
    {
        return false;
    }

    if (events_.empty())
    {
        unlock();
        return false;
    }

    // Events are transmitted in the same order they were generated.
    event = events_.front();
    events_.pop_front();

    unlock();
    return true;
}

bool OfflineEventStore::peek(Event& event)
{
    if (!lock())
    {
        return false;
    }

    if (events_.empty())
    {
        unlock();
        return false;
    }

    event = events_.front();

    unlock();
    return true;
}

void OfflineEventStore::clear()
{
    if (!lock())
    {
        return;
    }

    events_.clear();
    coalescedCount_ = 0;

    unlock();
}

std::size_t OfflineEventStore::size()
{
    if (!lock())
    {
        return 0;
    }

    const std::size_t currentSize = events_.size();

    unlock();

    return currentSize;
}

std::size_t OfflineEventStore::capacity() const
{
    return capacity_;
}

bool OfflineEventStore::empty()
{
    if (!lock())
    {
        return true;
    }

    const bool isEmpty = events_.empty();

    unlock();

    return isEmpty;
}

bool OfflineEventStore::full()
{
    if (!lock())
    {
        return true;
    }

    const bool isFull = events_.size() >= capacity_;

    unlock();

    return isFull;
}

std::size_t OfflineEventStore::coalescedCount()
{
    if (!lock())
    {
        return 0;
    }

    const std::size_t count = coalescedCount_;

    unlock();

    return count;
}

bool OfflineEventStore::lock()
{
    if (mutex_ == nullptr)
    {
        return false;
    }

    return xSemaphoreTake(
        mutex_,
        portMAX_DELAY
    ) == pdTRUE;
}

void OfflineEventStore::unlock()
{
    if (mutex_ != nullptr)
    {
        xSemaphoreGive(mutex_);
    }
}