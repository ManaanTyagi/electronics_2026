/*
  WIRELESS E-STOP - RECEIVER (RX)
  Hardware : ESP32 + Relay Module + Contactor
  Protocol : ESP-NOW
  Behavior : Relay stays ENERGIZED (contactor ON = system running).
             Relay DE-ENERGIZES (contactor OFF = STOP) when:
               1. E-Stop button is pressed on TX
               2. Heartbeat signal is lost for > 500ms (fail-safe)
  Wiring   : Relay IN pin -> RELAY_PIN
             Use relay NC (Normally Closed) terminal to contactor coil
             so power cut = contactor opens = machine stops
*/

#include <esp_now.h>
#include <WiFi.h>

#define RELAY_PIN          5    // Relay control pin
#define LED_GREEN         25    // Green = system running
#define LED_RED           26    // Red   = E-Stop active
#define RESET_BUTTON_PIN  18    // Physical reset button on receiver side

// How long (ms) without a heartbeat before E-Stop fires automatically
#define HEARTBEAT_TIMEOUT 500

// Message structure - must match TX exactly
typedef struct {
  bool estop;
  bool heartbeat;
} EStopMessage;

EStopMessage inMsg;
volatile bool estopActive    = false;
volatile unsigned long lastHeartbeat = 0;

// ----------------------------------------------------------
// Relay helpers
// Most relay modules are ACTIVE LOW (LOW = energized)
// Adjust if your module is active HIGH
// ----------------------------------------------------------
void relayON() {
  digitalWrite(RELAY_PIN, LOW);   // Energize relay -> contactor ON -> machine RUNS
}

void relayOFF() {
  digitalWrite(RELAY_PIN, HIGH);  // De-energize relay -> contactor OFF -> machine STOPS
}

// Callback: fires when a message is received via ESP-NOW
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(EStopMessage)) return;  // Ignore malformed packets

  memcpy(&inMsg, data, sizeof(inMsg));
  lastHeartbeat = millis();  // Reset heartbeat timer on every valid packet

  if (inMsg.estop) {
    estopActive = true;  // Latch E-Stop
    Serial.println("!!! E-STOP RECEIVED FROM TX !!!");
  }
}

void triggerEStop(const char *reason) {
  estopActive = true;
  relayOFF();
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);
  Serial.print("E-STOP ACTIVE: ");
  Serial.println(reason);
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN,        OUTPUT);
  pinMode(LED_GREEN,        OUTPUT);
  pinMode(LED_RED,          OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // Start safe - relay OFF until system is confirmed ready
  relayOFF();
  digitalWrite(LED_RED, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Print this ESP's MAC so you can put it in TX code
  Serial.print("RX MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init FAILED. Halting.");
    while (true);  // Halt - do not run without comms
  }

  esp_now_register_recv_cb(onReceive);

  // Wait for first heartbeat before enabling relay
  Serial.println("Waiting for TX heartbeat...");
  unsigned long waitStart = millis();
  while (lastHeartbeat == 0) {
    if (millis() - waitStart > 5000) {
      Serial.println("No TX found in 5s. Check TX power and MAC address.");
      waitStart = millis();  // Keep waiting, print warning every 5s
    }
    delay(100);
  }

  // TX confirmed - system ready
  estopActive = false;
  relayON();
  digitalWrite(LED_RED,   LOW);
  digitalWrite(LED_GREEN, HIGH);
  Serial.println("RX Ready - System Armed and Running");
}

void loop() {
  // -------------------------------------------------------
  // FAIL-SAFE: Heartbeat timeout check
  // If TX goes offline, battery dies, or signal lost -> STOP
  // -------------------------------------------------------
  if (!estopActive && (millis() - lastHeartbeat > HEARTBEAT_TIMEOUT)) {
    triggerEStop("HEARTBEAT LOST - TX offline or out of range");
  }

  // -------------------------------------------------------
  // E-Stop is LATCHED - physical reset button required
  // Press RESET_BUTTON_PIN to re-arm (ensures human confirms)
  // -------------------------------------------------------
  if (estopActive) {
    relayOFF();
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_GREEN, LOW);

    // Check if reset button is pressed AND heartbeat is healthy
    bool resetPressed    = (digitalRead(RESET_BUTTON_PIN) == LOW);
    bool heartbeatHealthy = (millis() - lastHeartbeat < HEARTBEAT_TIMEOUT);

    if (resetPressed && heartbeatHealthy && !inMsg.estop) {
      // Safe to reset: TX is alive and E-Stop button is released
      estopActive = false;
      relayON();
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, HIGH);
      Serial.println("System RESET - Running");
    }
  }

  delay(50);  // Check at 20Hz
}