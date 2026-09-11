# Testing and Validation

## 1. Objective

Testing will verify the operation, reliability, and recovery behaviour of the complete IoT visual-management station.

Testing covers:

- hardware interfaces
- embedded firmware
- networking
- MQTT
- offline operation
- backend communication
- Odoo integration
- bidirectional synchronization

---

## 2. Hardware Validation

Hardware testing shall verify:

- supply voltages
- GPIO voltage levels
- digital input behaviour
- magnetic sensor behaviour
- rotary encoder behaviour
- ADC ranges
- LED outputs
- PWM outputs where used
- communication stability
- signal integrity
- power stability

Appropriate bench and debugging equipment will be used where available.

---

## 3. Digital Input Testing

Digital interfaces shall be tested for:

- press detection
- release detection
- toggle state changes
- repeated activation
- rapid activation
- debounce performance
- noisy transitions
- invalid transitions

---

## 4. Rotary Encoder Testing

Encoder testing shall include:

- clockwise rotation
- counter-clockwise rotation
- correct count changes
- rapid movement
- repeated direction changes
- invalid quadrature transitions
- missed-transition behaviour

---

## 5. Analog Input Testing

Analog interfaces shall be tested for:

- minimum value
- maximum value
- ADC range
- calibration
- scaling
- stationary noise
- filtering
- deadband behaviour
- repeated movement

The control should not generate continuous state-change messages while physically stationary.

---

## 6. Output Testing

Output interfaces shall be tested for:

- local activation
- local deactivation
- remote activation
- remote deactivation
- correct mapping
- correct state after reconnect
- PWM operation where applicable

---

## 7. Wi-Fi Testing

Wi-Fi testing shall include:

- normal connection
- connection failure
- temporary disconnection
- automatic reconnection
- repeated disconnection
- weak-signal behaviour where practical

---

## 8. MQTT Testing

MQTT testing shall include:

- initial broker connection
- message publishing
- message subscription
- disconnect detection
- automatic reconnect
- topic resubscription
- malformed message handling
- duplicate message handling

---

## 9. Offline Operation Testing

Offline behaviour shall be tested using the following process:

```text
1. Establish normal operation
2. Disconnect Wi-Fi or MQTT
3. Generate multiple physical events
4. Verify local station operation continues
5. Verify events are buffered
6. Restore connectivity
7. Verify automatic reconnection
8. Verify MQTT subscriptions are restored
9. Verify buffered events are transmitted
10. Verify synchronization is restored
```

No intended operational event should be silently lost.

---

## 10. Reboot and Power-Cycle Testing

Testing shall include:

- normal ESP32 reboot
- unexpected reset
- full power cycle
- reboot while online
- reboot while offline
- recovery of persistent configuration
- recovery of relevant local state
- recovery of pending events where required

---

## 11. End-to-End Physical-to-Odoo Test

The complete forward communication path shall be validated.

```text
Physical Control
      |
      v
ESP32 Input Driver
      |
      v
Internal Event
      |
      v
State Manager
      |
      v
MQTT
      |
      v
Backend
      |
      v
Odoo
```

A physical action must produce the expected Odoo update.

---

## 12. End-to-End Odoo-to-Physical Test

The reverse communication path shall also be validated.

```text
Odoo
 |
 v
Backend
 |
 v
MQTT
 |
 v
ESP32
 |
 v
State Manager
 |
 v
Output Driver
 |
 v
Physical Indicator
```

An ERP-side state change must produce the intended physical response.

---

## 13. Fault Injection

Fault testing should include:

- Wi-Fi loss
- broker loss
- backend loss
- malformed JSON
- invalid command
- duplicate message
- device reboot
- device power cycle
- rapid repeated input events
- invalid input values

The expected response should be controlled recovery rather than undefined behaviour.

---

## 14. Performance Measurements

Where practical, the following measurements shall be collected:

- physical-event-to-ERP latency
- ERP-to-physical-indicator latency
- Wi-Fi reconnect time
- MQTT reconnect time
- event-loss rate
- number of supported interfaces
- successful recovery rate
- uptime during continuous testing

---

## 15. Test Documentation

Test results should record:

- test name
- test conditions
- expected result
- actual result
- pass/fail
- measured value where relevant
- observations
- corrective action if required

Measured results will be added after prototype implementation and validation.