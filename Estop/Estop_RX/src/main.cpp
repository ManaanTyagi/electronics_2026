/*
  WIRELESS E-STOP - RECEIVER (RX)
  Behavior : Receives heartbeat and continuous counter.
  Prints received numbers and current E-Stop state to Serial.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // Added for Long Range mode

#define RELAY_PIN              5   // Relay control pin
#define LED_GREEN              25  // Green = system running
#define LED_RED                26  // Red   = E-Stop active
#define RESET_BUTTON_PIN       18  // Physical reset button on receiver
#define LOCAL_HARDWARE_ESTOP   17  // Connect to 3.3V to trigger E-Stop locally on rover side

#define HEARTBEAT_TIMEOUT     500  // ms before heartbeat loss triggers E-Stop

// Message structure - MUST MATCH TX EXACTLY
typedef struct {
  bool estop;
  bool heartbeat;
  bool hwEstop;
  long counter;    // Changed to long to match TX
} EStopMessage;

EStopMessage inMsg;
volatile bool estopActive         = false;
volatile bool hwEstopActive       = false;
volatile unsigned long lastHeartbeat = 0;

void relayON() {
  digitalWrite(RELAY_PIN, LOW);
}

void relayOFF() {
  digitalWrite(RELAY_PIN, HIGH);
}

void triggerEStop(const char *reason) {
  if (!estopActive) { // Only print the trigger reason once
    Serial.print("\n!!! E-STOP TRIGGERED: ");
    Serial.println(reason);
  }
  estopActive = true;
  relayOFF();
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);
}

// Callback for ESP32 Core v3.x
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(EStopMessage)) return;
  memcpy(&inMsg, data, sizeof(inMsg));
  lastHeartbeat = millis();

  // Print incoming data continuously
  Serial.print("RX | System State: ");
  Serial.print(estopActive ? "TRIGGERED" : "RUNNING  ");
  Serial.print(" | Received Counter: ");
  Serial.println(inMsg.counter);

  if (inMsg.hwEstop && !estopActive) {
    triggerEStop("TX HARDWARE PIN TRIGGERED");
    hwEstopActive = true;
  }

  if (inMsg.estop && !estopActive) {
    triggerEStop("TX BUTTON PRESSED");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN,              OUTPUT);
  pinMode(LED_GREEN,              OUTPUT);
  pinMode(LED_RED,                OUTPUT);
  pinMode(RESET_BUTTON_PIN,       INPUT_PULLUP);
  pinMode(LOCAL_HARDWARE_ESTOP,   INPUT);

  relayOFF();
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // --- NEW ADDITIONS FOR MAXIMUM RANGE ---
  // Enable ESP32 Long Range (LR) Protocol to match Transmitter
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  // Force Maximum Transmit Power (19.5 dBm) for robust ACKs
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  // ---------------------------------------

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init FAILED. Halting.");
    while (true);
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("Waiting for TX heartbeat...");
  unsigned long waitStart = millis();
  while (lastHeartbeat == 0) {
    if (millis() - waitStart > 5000) {
      Serial.println("No TX found in 5s. Check TX power and MAC address.");
      waitStart = millis();
    }
    delay(100);
  }

  estopActive = false;
  relayON();
  digitalWrite(LED_RED,   LOW);
  digitalWrite(LED_GREEN, HIGH);
  Serial.println("RX Ready - LR Mode Enabled - System Armed and Running");
}

void loop() {

  // Check LOCAL hardware E-Stop
  if (digitalRead(LOCAL_HARDWARE_ESTOP) == HIGH && !estopActive) {
    triggerEStop("LOCAL HARDWARE PIN ON ROVER TRIGGERED");
    hwEstopActive = true;
  }

  // FAIL-SAFE: Heartbeat timeout
  if (!estopActive && (millis() - lastHeartbeat > HEARTBEAT_TIMEOUT)) {
    triggerEStop("HEARTBEAT LOST - TX offline or out of range");
  }

  // E-Stop LATCH Reset Logic
  if (estopActive) {
    relayOFF();
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_GREEN, LOW);

    bool resetPressed     = (digitalRead(RESET_BUTTON_PIN)     == LOW);
    bool heartbeatHealthy = (millis() - lastHeartbeat < HEARTBEAT_TIMEOUT);
    bool txEstopCleared   = (!inMsg.estop && !inMsg.hwEstop);
    bool localPinClear    = (digitalRead(LOCAL_HARDWARE_ESTOP) == LOW);

    if (resetPressed && heartbeatHealthy && txEstopCleared && localPinClear) {
      estopActive   = false;
      hwEstopActive = false;
      relayON();
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, HIGH);
      Serial.println("\nSystem RESET - Running");
    } else if (resetPressed) {
      Serial.println("\nReset BLOCKED. Ensure all E-Stops are released and TX is online.");
    }
  }

  delay(50);
}