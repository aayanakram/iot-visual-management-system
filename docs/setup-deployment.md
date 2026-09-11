# Setup and Deployment

## 1. Overview

This document describes the planned development, setup, configuration, and deployment process for the IoT-connected visual management station.

The system consists of:

- ESP32-S3 embedded hardware
- ESP-IDF firmware written in C++
- FreeRTOS-based application tasks
- Wi-Fi connectivity
- MQTT communication
- backend/middleware services
- Odoo ERP integration

The deployment procedure will be updated as the prototype implementation is completed.

---

## 2. Development Requirements

The firmware development environment requires:

- ESP32-S3 development board
- USB cable for programming and debugging
- ESP-IDF
- C++ toolchain
- Git
- serial terminal
- Wi-Fi network
- MQTT broker
- backend development environment
- Odoo development or test environment

Recommended debugging equipment may include:

- digital multimeter
- oscilloscope
- logic analyzer

depending on hardware availability.

---

## 3. Repository Structure

The project repository is organized as follows:

```text
iot-visual-management-station/
|
+-- backend/
|
+-- docs/
|
+-- firmware/
|
+-- hardware/
|
+-- tests/
|
+-- README.md
```

### backend/

Contains middleware used to connect MQTT station messages with Odoo.

### docs/

Contains:

- system requirements
- system architecture
- firmware architecture
- MQTT integration documentation
- Odoo integration documentation
- testing documentation
- known limitations
- deployment instructions

### firmware/

Contains the ESP-IDF firmware for the ESP32-S3.

### hardware/

Contains hardware design information including:

- schematics
- pin mappings
- bill of materials
- component documentation

### tests/

Contains test procedures, scripts, and recorded validation results where applicable.

---

## 4. ESP-IDF Installation

The firmware is developed using Espressif's ESP-IDF framework.

ESP-IDF should be installed using the official Espressif installation procedure for the host operating system.

After installation, verify that the ESP-IDF development environment is available:

```bash
idf.py --version
```

The exact ESP-IDF version used for the final prototype should be recorded before final deployment.

---

## 5. Firmware Setup

Navigate to the firmware directory:

```bash
cd firmware
```

Set the project target to ESP32-S3:

```bash
idf.py set-target esp32s3
```

Configure the project where required:

```bash
idf.py menuconfig
```

Configuration options may include:

- Wi-Fi support
- MQTT support
- TLS settings
- logging
- FreeRTOS configuration
- flash configuration
- partition configuration
- watchdog configuration

---

## 6. Firmware Build

Build the ESP-IDF project using:

```bash
idf.py build
```

The build process should complete without errors before firmware is flashed to the device.

---

## 7. Flashing the ESP32-S3

Connect the ESP32-S3 to the development computer using USB.

Flash the firmware using:

```bash
idf.py flash
```

If a serial port must be selected manually:

```bash
idf.py -p <PORT> flash
```

Example:

```bash
idf.py -p COM5 flash
```

The exact port depends on the development computer.

---

## 8. Serial Monitoring

ESP-IDF serial monitoring can be started using:

```bash
idf.py monitor
```

Flashing and monitoring can also be performed in one command:

```bash
idf.py flash monitor
```

Serial logs should provide information including:

- system startup
- firmware version
- device ID
- input initialization
- Wi-Fi connection status
- MQTT connection status
- event processing
- synchronization
- errors
- recovery operations

Sensitive credentials must not be printed in logs.

---

## 9. Device Configuration

Each physical station requires station-specific configuration.

Configuration may include:

- unique device ID
- station name
- Wi-Fi settings
- MQTT broker settings
- MQTT credentials
- input mappings
- output mappings
- backend configuration
- station-specific behaviour

Where appropriate, persistent configuration shall be stored using ESP32 NVS.

The firmware should avoid requiring source-code changes for normal station-specific configuration.

---

## 10. Device Identification

Each station shall have a unique device identifier.

Example:

```text
station_01
station_02
station_03
```

The device ID is used for:

- MQTT topic routing
- backend identification
- telemetry
- configuration
- ERP mapping

Device IDs must not be duplicated between active stations.

---

## 11. Wi-Fi Configuration

The station requires access to an appropriate Wi-Fi network.

The firmware shall support:

- initial connection
- connection monitoring
- automatic reconnection
- operation during temporary network loss

Production Wi-Fi credentials must not be committed to the public source repository.

The final Wi-Fi provisioning method remains to be finalized.

---

## 12. MQTT Configuration

Each device requires access to an MQTT broker.

Required configuration may include:

- broker hostname or IP address
- broker port
- device ID
- authentication credentials
- TLS configuration
- topic configuration

The planned topic structure is:

```text
kaizen/stations/{device_id}/events
kaizen/stations/{device_id}/state
kaizen/stations/{device_id}/telemetry
kaizen/stations/{device_id}/heartbeat
kaizen/stations/{device_id}/commands
kaizen/stations/{device_id}/config
```

Production MQTT communication should use TLS.

---

## 13. Backend Setup

The backend/middleware is responsible for connecting device MQTT communication with Odoo.

The backend may perform:

- MQTT subscription
- MQTT publishing
- message validation
- device identification
- message routing
- duplicate detection
- device-to-Odoo mapping
- Odoo authentication
- Odoo API requests
- synchronization
- retry handling
- logging

Final backend setup instructions will be added after the middleware implementation is completed.

---

## 14. Odoo Setup

A development or test Odoo environment should be used during integration testing.

The final setup will require:

- Odoo server address
- authentication credentials
- database or instance information
- selected API mechanism
- required models
- required fields
- station-to-Odoo mappings

Potential integration methods include:

- JSON-RPC
- XML-RPC
- REST/custom API
- custom Odoo module

The final method remains to be confirmed.

---

## 15. Physical Interface Setup

Before deploying a station, each physical interface must be mapped to the corresponding firmware input or output.

Interfaces may include:

- toggle switches
- magnetic sensors
- push buttons
- rotary encoders
- analog sliders
- LEDs
- status indicators

The final hardware interface map shall document:

- interface ID
- interface type
- GPIO or peripheral
- expected signal type
- associated physical control
- associated station function

---

## 16. Initial Device Bring-Up

The recommended initial bring-up sequence is:

```text
1. Verify power supply
2. Connect ESP32-S3
3. Flash firmware
4. Open serial monitor
5. Confirm device initialization
6. Verify physical inputs
7. Verify physical outputs
8. Connect to Wi-Fi
9. Connect to MQTT broker
10. Verify MQTT publishing
11. Verify MQTT subscription
12. Verify backend communication
13. Verify Odoo integration
14. Perform bidirectional system test
```

---

## 17. Hardware Validation Before Deployment

Before deployment, verify:

- correct supply voltage
- stable power
- correct GPIO behaviour
- correct ADC ranges
- reliable encoder operation
- reliable sensor operation
- correct LED/output behaviour
- communication stability
- secure electrical connections

Any interface faults should be resolved before connecting the station to the production backend.

---

## 18. Connectivity Validation

After network configuration, verify:

```text
ESP32 -> Wi-Fi
ESP32 -> MQTT Broker
MQTT Broker -> Backend
Backend -> Odoo
```

The reverse communication path must also be verified:

```text
Odoo -> Backend
Backend -> MQTT Broker
MQTT Broker -> ESP32
ESP32 -> Physical Output
```

---

## 19. Offline Recovery Validation

Before deployment, temporary connectivity loss should be tested.

Procedure:

```text
1. Establish normal operation
2. Disconnect network connectivity
3. Generate physical station events
4. Verify local operation continues
5. Verify events are buffered
6. Restore network connectivity
7. Verify automatic reconnection
8. Verify MQTT subscriptions are restored
9. Verify buffered events are transmitted
10. Verify local and ERP state synchronization
```

---

## 20. Production Deployment

A final station deployment should include:

```text
Hardware assembly
        |
        v
Electrical validation
        |
        v
Firmware flashing
        |
        v
Device configuration
        |
        v
Wi-Fi configuration
        |
        v
MQTT configuration
        |
        v
Backend registration
        |
        v
Odoo mapping
        |
        v
Functional validation
        |
        v
Offline/recovery test
        |
        v
Station ready for operation
```

---

## 21. Updating Firmware

During development, firmware may be updated through USB using ESP-IDF.

```bash
idf.py flash
```

OTA firmware updates are planned as a future or extended deployment capability.

If OTA functionality is implemented, deployment documentation will be expanded to cover:

- remote update initiation
- firmware version validation
- firmware integrity checking
- update success verification
- rollback or recovery behaviour

---

## 22. Security Considerations

The following must not be committed to the source repository:

- Wi-Fi passwords
- MQTT passwords
- Odoo passwords
- API tokens
- TLS private keys
- production secrets

Example or placeholder configuration values may be provided for development documentation.

Production credentials should be provisioned separately.

---

## 23. Deployment Checklist

Before a station is considered ready for deployment, verify:

- [ ] hardware connections inspected
- [ ] supply voltage verified
- [ ] firmware builds successfully
- [ ] firmware flashed successfully
- [ ] correct device ID configured
- [ ] input interfaces verified
- [ ] output interfaces verified
- [ ] Wi-Fi connection verified
- [ ] MQTT connection verified
- [ ] MQTT publishing verified
- [ ] MQTT subscription verified
- [ ] backend communication verified
- [ ] Odoo update verified
- [ ] Odoo-to-station update verified
- [ ] offline buffering tested
- [ ] automatic reconnection tested
- [ ] synchronization recovery tested
- [ ] serial logs checked for unexpected faults

---

## 24. Current Deployment Limitations

The following items remain under development or require confirmation:

- final physical interface allocation
- production power architecture
- final MQTT broker
- production credential provisioning
- final Odoo API mechanism
- final configuration interface
- final offline reconciliation policy
- OTA firmware update implementation
- formal environmental protection requirements

This document will be updated as these items are finalized.