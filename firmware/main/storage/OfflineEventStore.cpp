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

    // Keep the buffer bounded so disconnected operation cannot consume memory indefinitely.
    if (events_.size() >= capacity_)
    {
        unlock();
        return false;
    }

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