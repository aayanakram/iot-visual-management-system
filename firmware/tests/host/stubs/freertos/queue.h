#pragma once
#include "FreeRTOS.h"
#include <cstring>
#include <deque>
#include <vector>
extern std::int64_t testTimeUs;
extern int idleReceivesRemaining;
struct StopHostLoop {};
struct HostQueue { unsigned capacity; unsigned itemSize; std::deque<std::vector<char>> items; };
using QueueHandle_t = HostQueue*;
inline QueueHandle_t xQueueCreate(unsigned capacity, unsigned size) { return new HostQueue{capacity, size, {}}; }
inline void vQueueDelete(QueueHandle_t q) { delete q; }
inline int xQueueSend(QueueHandle_t q, const void* item, unsigned) {
    if (q->items.size() == q->capacity) return 0;
    const char* bytes = static_cast<const char*>(item);
    q->items.emplace_back(bytes, bytes + q->itemSize);
    return pdPASS;
}
inline int xQueueReceive(QueueHandle_t q, void* item, unsigned timeout) {
    if (q->items.empty()) {
        if (timeout) {
            if (idleReceivesRemaining-- == 0) throw StopHostLoop{};
            testTimeUs += static_cast<std::int64_t>(timeout) * 1000;
        }
        return 0;
    }
    std::memcpy(item, q->items.front().data(), q->itemSize);
    q->items.pop_front();
    return pdPASS;
}
