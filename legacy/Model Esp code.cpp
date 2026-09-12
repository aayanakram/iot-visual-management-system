#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Industry standard for parsing IoT payloads

// --- 1. Network & MQTT Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "192.168.1.100"; // The IP of your MQTT Broker or ERP
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// --- 2. Hardware Pin Definitions ---
const int TOGGLE_PIN = 25;   // Module 1: Task Status (Digital)
const int POT_PIN = 34;      // Module 2: Rotary/Slider (Analog ADC)
const int BUTTON_PIN = 26;   // Module 3: Alert Button (Digital)
const int LED_PIN = 27;      // ERP Feedback LED (Digital)

// --- 3. State Management (Prevents network spam) ---
int lastToggleState = -1;
int lastPotPercent = -1;
int lastButtonState = HIGH; 
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// --- 4. WiFi Setup ---
void setup_wifi() {
  delay(10);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected successfully.");
}

// --- 5. Downstream: Receiving Data FROM the ERP ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert incoming payload byte array to a String
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.println("Payload: " + message);

  // Parse the JSON payload from the ERP
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("Failed to parse ERP JSON payload");
    return;
  }

  // If the ERP is talking to the LED module
  if (String(topic) == "station1/led_feedback") {
    String color = doc["color"];
    String state = doc["state"];
    
    if (state == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("ERP COMMAND: LED Turned ON");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("ERP COMMAND: LED Turned OFF");
    }
  }
}

// --- 6. Reconnection Strategy ---
void reconnect() {
  // Loop until we're reconnected (Handles connection loss per JD requirements)
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("Kaizen_ESP32_Station1")) {
      Serial.println("Connected to Broker.");
      // Subscribe to topics the ERP will publish to
      client.subscribe("station1/led_feedback");
      client.subscribe("erp/workorder/101");
    } else {
      Serial.print("Failed, state=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5 seconds.");
      delay(5000);
    }
  }
}

// --- 7. Main Setup ---
void setup() {
  Serial.begin(115200);
  
  // Initialize Hardware Pins
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  // Note: POT_PIN (Analog 34) does not need a pinMode declaration on ESP32

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

// --- 8. Main Loop (Upstream Data Collection) ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Keeps MQTT connection alive

  StaticJsonDocument<200> jsonDoc;
  char jsonBuffer[256];

  // --- Module 1: Read Toggle Switch ---
  int toggleState = digitalRead(TOGGLE_PIN);
  if (toggleState != lastToggleState) {
    jsonDoc.clear();
    jsonDoc["status"] = (toggleState == LOW) ? "Done" : "To Do"; // LOW because of pullup
    serializeJson(jsonDoc, jsonBuffer);
    
    client.publish("station1/module1/status", jsonBuffer);
    lastToggleState = toggleState;
    delay(50); // Basic physical debounce
  }

  // --- Module 2: Read Rotary/Slider (Analog) ---
  // ESP32 ADC is 12-bit (0-4095). Map it to a 0-100% KPI value.
  int potValue = analogRead(POT_PIN);
  int potPercent = map(potValue, 0, 4095, 0, 100);
  
  // Only transmit if the value changed by >= 2% to prevent network spamming
  if (abs(potPercent - lastPotPercent) >= 2) {
    jsonDoc.clear();
    jsonDoc["completion"] = potPercent;
    serializeJson(jsonDoc, jsonBuffer);
    
    client.publish("station1/module2/kpi", jsonBuffer);
    lastPotPercent = potPercent;
  }

  // --- Module 3: Read Push Button (With strict Debounce) ---
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button is pressed (LOW due to internal pullup resistor)
    if (reading == LOW) {
      jsonDoc.clear();
      jsonDoc["alert"] = "TRIGGERED";
      serializeJson(jsonDoc, jsonBuffer);
      
      client.publish("station1/module3/alert", jsonBuffer);
      Serial.println("HARDWARE: Alert Button Pressed!");
      
      delay(500); // Wait half a second so a single press doesn't fire 10 times
    }
  }
  lastButtonState = reading;
}