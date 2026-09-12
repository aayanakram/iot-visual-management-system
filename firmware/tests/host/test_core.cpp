#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include "cJSON.h"
#include "MessageSerializer.hpp"
#include "DigitalInput.hpp"
#include "RotaryEncoder.hpp"
// Expose orchestration methods only to this test translation unit.
#define private public
#include "Application.hpp"
#undef private

std::int64_t testTimeUs = 1234000;
int idleReceivesRemaining = 0;
int testGpio[50] = {};

static Event toggle(int value = 1, unsigned sequence = 1) {
    Event e;
    e.type = EventType::ToggleChanged; e.sourceId = 2;
    e.value = value; e.sequenceId = sequence; e.timestampMs = 1234;
    return e;
}

// Event from an arbitrary logical source; the offline store keys coalescing on
// sourceId, so tests need sources other than the toggle's.
static Event input(EventType type, unsigned source, int value, unsigned sequence = 0) {
    Event e;
    e.type = type; e.sourceId = source;
    e.value = value; e.sequenceId = sequence; e.timestampMs = 1234;
    return e;
}

// cJSON allocation hooks so a serialization failure can be induced deliberately.
static int failNextAllocations = 0;

static void* testMalloc(std::size_t size) {
    if (failNextAllocations > 0) { --failNextAllocations; return nullptr; }
    return std::malloc(size);
}

static void testFree(void* pointer) { std::free(pointer); }

static void serialization() {
    const auto json = MessageSerializer::serializeEvent(toggle(), "station_01", "0.1.0");
    cJSON* root = cJSON_Parse(json.c_str());
    assert(root && cJSON_GetArraySize(root) == 7);
    assert(std::string(cJSON_GetObjectItem(root, "event_type")->valuestring) == "toggle_changed");
    assert(cJSON_GetObjectItem(root, "timestamp_ms")->valuedouble == 1234);
    assert(cJSON_GetObjectItem(root, "sequence_id")->valuedouble == 1);
    cJSON_Delete(root);
    Event command;
    for (const char* kind : {"set_output", "set_state"}) {
        for (const char* value : {"true", "false", "0", "1", "1.0"}) {
            assert(MessageSerializer::parseCommand(std::string("{\"command\":\"") + kind + "\",\"value\":" + value + "}", command));
            assert(command.timestampMs == 1234 && command.sequenceId == 0);
        }
    }
    for (const char* bad : {"{}", "[]", "null", "{", "{\"command\":\"reboot\",\"value\":true}",
         "{\"command\":\"set_output\",\"value\":2}", "{\"command\":\"set_output\",\"value\":0.5}",
         "{\"command\":\"set_output\",\"value\":\"true\"}", "{\"command\":\"set_output\",\"value\":null}",
         "{\"command\":\"set_output\",\"value\":true}junk",
         "{\"command\":\"set_output\",\"value\":true,\"value\":false}",
         "{\"command\":\"set_output\",\"value\":true,\"output_id\":1}"}) {
        command.value = 99;
        assert(!MessageSerializer::parseCommand(bad, command));
        assert(command.value == 99);
    }
    assert(!MessageSerializer::parseCommand(json + std::string(1, '\0'), command));
    assert(!MessageSerializer::parseCommand(std::string(513, ' '), command));
}

static void offlineStore() {
    OfflineEventStore store(2);
    Event e;
    assert(store.capacity() == 2 && store.empty() && !store.pop(e));
    // Distinct sources stay as separate entries and keep their arrival order.
    assert(store.push(input(EventType::EncoderChanged, 3, 10, 5)));
    assert(store.push(input(EventType::AnalogChanged, 4, 20, 6)));
    // A third source has nothing to replace, so the bound still applies.
    assert(store.full() && !store.push(input(EventType::ButtonPressed, 1, 1, 7)));
    assert(store.peek(e) && e.sequenceId == 5 && store.size() == 2);
    assert(store.pop(e) && e.sequenceId == 5);
    assert(store.pop(e) && e.sequenceId == 6 && store.empty());
    store.push(toggle()); store.clear();
    assert(store.empty() && store.coalescedCount() == 0);
    OfflineEventStore rebooted(2); assert(rebooted.empty());
    OfflineEventStore zero(0); assert(!zero.push(toggle()));

    // Absolute state: a newer value replaces the older one from the same source
    // and moves to the back, so a full store still accepts the current value.
    OfflineEventStore coalescing(2);
    assert(coalescing.push(input(EventType::EncoderChanged, 3, 10, 1)));
    assert(coalescing.push(input(EventType::AnalogChanged, 4, 55, 2)));
    assert(coalescing.full());
    assert(coalescing.push(input(EventType::AnalogChanged, 4, 99, 3)));
    assert(coalescing.size() == 2 && coalescing.coalescedCount() == 1);
    assert(coalescing.pop(e) && e.sequenceId == 1 && e.sourceId == 3);
    assert(coalescing.pop(e) && e.sequenceId == 3 && e.value == 99);
    assert(coalescing.empty());

    // A slider sweep during an outage collapses to its current value instead of
    // overflowing and leaving only the stale values from the start of the outage.
    OfflineEventStore sweep(64);
    for (int percent = 0; percent <= 100; ++percent) {
        assert(sweep.push(input(EventType::AnalogChanged, 4, percent,
                                static_cast<unsigned>(percent) + 1)));
    }
    assert(sweep.size() == 1 && sweep.coalescedCount() == 100);
    assert(sweep.peek(e) && e.value == 100 && e.sequenceId == 101);

    // System-source events are discrete occurrences, so they are never coalesced.
    OfflineEventStore faults(2);
    assert(faults.push(input(EventType::FaultDetected, 0, 7, 1)));
    assert(faults.push(input(EventType::FaultDetected, 0, 8, 2)));
    assert(faults.size() == 2 && faults.coalescedCount() == 0);
    assert(!faults.push(input(EventType::FaultDetected, 0, 9, 3)));
}

static void offlineCoalescingThroughApplication() {
    EventQueue queue(8);
    StateManager state;
    LedDriver led(2, true);
    OutputManager output(led);
    assert(output.initialize());
    MqttManager mqtt;
    OfflineEventStore store(64);
    Application app(queue, state, output, mqtt, store);

    // A full slider sweep while MQTT is unavailable: 101 events, one source.
    for (int percent = 0; percent <= 100; ++percent) {
        app.processEvent(input(EventType::AnalogChanged, 4, percent));
    }
    assert(store.size() == 1 && store.coalescedCount() == 100);
    assert(state.analogValue() == 100);

    // Replay carries the station's current value, not the one it held when the
    // outage began, and keeps the sequence ID assigned at generation time.
    mqtt.connected = true;
    app.flushOfflineEvents();
    assert(store.empty() && mqtt.payloads.size() == 1);
    cJSON* root = cJSON_Parse(mqtt.payloads.back().c_str());
    assert(cJSON_GetObjectItem(root, "value")->valuedouble == 100);
    assert(cJSON_GetObjectItem(root, "sequence_id")->valuedouble == 101);
    cJSON_Delete(root);
}

static void flushSurvivesSerializeFailure() {
    EventQueue queue(8);
    StateManager state;
    LedDriver led(2, true);
    OutputManager output(led);
    assert(output.initialize());
    MqttManager mqtt;
    OfflineEventStore store(4);
    Application app(queue, state, output, mqtt, store);

    // Three distinct sources so nothing coalesces away before the flush.
    assert(store.push(input(EventType::ButtonPressed, 1, 1, 11)));
    assert(store.push(input(EventType::EncoderChanged, 3, 42, 12)));
    assert(store.push(input(EventType::AnalogChanged, 4, 60, 13)));
    mqtt.connected = true;

    // Fail the first cJSON allocation so exactly the head event cannot serialize.
    cJSON_Hooks hooks;
    hooks.malloc_fn = testMalloc;
    hooks.free_fn = testFree;
    cJSON_InitHooks(&hooks);
    failNextAllocations = 1;
    app.flushOfflineEvents();
    cJSON_InitHooks(nullptr);
    failNextAllocations = 0;

    // The unserializable head is dropped rather than stalling everything behind it.
    assert(store.empty() && mqtt.payloads.size() == 2);
    for (const auto& payload : mqtt.payloads) {
        assert(payload.find("\"sequence_id\":11") == std::string::npos);
    }
}

static void applicationFlow() {
    EventQueue queue(32);
    StateManager state;
    LedDriver led(2, true);
    OutputManager output(led);
    assert(output.initialize());
    MqttManager mqtt;
    OfflineEventStore store(3);
    Application app(queue, state, output, mqtt, store);
    // Distinct sources, so this exercises ordering rather than coalescing.
    app.processEvent(toggle(1));
    app.processEvent(input(EventType::AnalogChanged, 4, 60));
    assert(state.toggleState() && store.size() == 2);
    mqtt.connected = true; mqtt.acceptedBeforeFailure = 1;
    app.flushOfflineEvents();
    assert(store.size() == 1 && mqtt.payloads.size() == 1);
    app.processEvent(input(EventType::EncoderChanged, 3, 7));
    assert(store.size() == 2); // newer event did not bypass a failed older event
    mqtt.acceptedBeforeFailure = -1;
    idleReceivesRemaining = 1;
    try { app.run(); } catch (const StopHostLoop&) {}
    assert(store.empty() && mqtt.payloads.size() == 3);
    for (unsigned i = 0; i < 3; ++i) {
        cJSON* root = cJSON_Parse(mqtt.payloads[i].c_str());
        assert(cJSON_GetObjectItem(root, "sequence_id")->valuedouble == i + 1);
        assert(cJSON_GetObjectItem(root, "timestamp_ms")->valuedouble == 1234);
        cJSON_Delete(root);
        assert(mqtt.topics[i] == "kaizen/stations/station_01/events");
    }
    Event command;
    assert(MessageSerializer::parseCommand("{\"command\":\"set_state\",\"value\":true}", command));
    app.processEvent(command);
    assert(state.outputState() && led.state() && testGpio[2] == 1);
    assert(mqtt.payloads.size() == 3); // commands are not echoed upstream
    testTimeUs = 0; idleReceivesRemaining = 30;
    try { app.run(); } catch (const StopHostLoop&) {}
    assert(mqtt.payloads.size() == 4);
    cJSON* root = cJSON_Parse(mqtt.payloads.back().c_str());
    assert(std::string(cJSON_GetObjectItem(root, "event_type")->valuestring) == "heartbeat");
    assert(cJSON_GetObjectItem(root, "value")->valuedouble == 123456);
    assert(cJSON_GetObjectItem(root, "timestamp_ms")->valuedouble == 30000);
    cJSON_Delete(root);
    mqtt.connected = false;
    Event heartbeat; heartbeat.type = EventType::Heartbeat;
    app.processEvent(heartbeat);
    assert(store.empty());
}

static void digitalAndEncoder() {
    EventQueue queue(8); Event e;
    testTimeUs = 0; testGpio[4] = 1;
    DigitalInput input(4, 1, EventType::ButtonPressed, EventType::ButtonReleased, queue, 30);
    assert(input.initialize());
    input.update(); assert(!queue.receive(e, 0));
    testTimeUs = 30000; input.update();
    assert(queue.receive(e, 0) && e.type == EventType::ButtonReleased);
    testGpio[4] = 0; testTimeUs = 40000; input.update();
    testGpio[4] = 1; testTimeUs = 50000; input.update();
    testGpio[4] = 0; testTimeUs = 60000; input.update();
    testTimeUs = 89000; input.update(); assert(!queue.receive(e, 0));
    testTimeUs = 90000; input.update();
    assert(queue.receive(e, 0) && e.type == EventType::ButtonPressed && e.timestampMs == 90);
    testGpio[6] = 0; testGpio[7] = 0;
    RotaryEncoder encoder(6, 7, 3, queue); assert(encoder.initialize());
    for (int bits : {2, 3, 1, 0}) {
        testGpio[6] = bits >> 1; testGpio[7] = bits & 1; encoder.update();
    }
    assert(encoder.count() == 4);
    for (int bits : {1, 3, 2, 0}) {
        testGpio[6] = bits >> 1; testGpio[7] = bits & 1; encoder.update();
    }
    assert(encoder.count() == 0);
    testGpio[6] = 1; testGpio[7] = 1; encoder.update(); assert(encoder.count() == 0);
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--event") {
        std::cout << MessageSerializer::serializeEvent(toggle(), "station_01", "0.1.0") << '\n';
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--command") {
        Event e;
        if (!MessageSerializer::parseCommand(argv[2], e)) return 2;
        std::cout << e.value << '\n'; return 0;
    }
    serialization(); offlineStore(); offlineCoalescingThroughApplication();
    flushSurvivesSerializeFailure(); applicationFlow(); digitalAndEncoder();
    std::cout << "PASS: serialization, commands, offline FIFO coalescing, unserializable-event drain, "
                 "application recovery/heartbeat, state/output, digital debounce, quadrature\n";
}
