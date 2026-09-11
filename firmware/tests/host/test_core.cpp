#include <cassert>
#include <cstdint>
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
    assert(store.push(toggle(1, 5)) && store.push(toggle(0, 6)));
    assert(store.full() && !store.push(toggle(1, 7)));
    assert(store.peek(e) && e.sequenceId == 5 && store.size() == 2);
    assert(store.pop(e) && e.sequenceId == 5);
    assert(store.pop(e) && e.sequenceId == 6 && store.empty());
    store.push(toggle()); store.clear(); assert(store.empty());
    OfflineEventStore rebooted(2); assert(rebooted.empty());
    OfflineEventStore zero(0); assert(!zero.push(toggle()));
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
    app.processEvent(toggle(1));
    app.processEvent(toggle(0));
    assert(!state.toggleState() && store.size() == 2);
    mqtt.connected = true; mqtt.acceptedBeforeFailure = 1;
    app.flushOfflineEvents();
    assert(store.size() == 1 && mqtt.payloads.size() == 1);
    app.processEvent(toggle(1));
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
    serialization(); offlineStore(); applicationFlow(); digitalAndEncoder();
    std::cout << "PASS: serialization, commands, offline FIFO, application recovery/heartbeat, state/output, digital debounce, quadrature\n";
}
