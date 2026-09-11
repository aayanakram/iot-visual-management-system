#pragma once
#include <string>
#include <vector>
// Only the Application-facing transport is replaced; no network is exercised.
class MqttManager {
public:
    bool connected = false;
    int acceptedBeforeFailure = -1;
    std::vector<std::string> payloads;
    std::vector<std::string> topics;
    bool isConnected() const { return connected; }
    bool publish(const std::string& topic, const std::string& payload, int, bool) {
        if (!connected || acceptedBeforeFailure == 0) return false;
        if (acceptedBeforeFailure > 0) --acceptedBeforeFailure;
        topics.push_back(topic);
        payloads.push_back(payload);
        return true;
    }
};
