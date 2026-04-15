/*
  WIRELESS E-STOP - TRANSMITTER (TX)
  Hardware : ESP32 + Momentary Push Button
  Protocol : ESP-NOW
  Behavior : Sends heartbeat every 100ms. On button press,
             sends E-Stop flag. Receiver fails safe on signal loss.
*/

#include <esp_now.h>
#include <WiFi.h>

// -------------------------------------------------------
// !! REPLACE WITH YOUR RECEIVER ESP32 MAC ADDRESS !!
// -------------------------------------------------------
uint8_t receiverMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

#define BUTTON_PIN     4    // E-Stop button pin (use momentary NO button)
#define LED_GREEN      25   // Green = system running
#define LED_RED        26   // Red   = E-Stop active

// Message structure shared by TX and RX (must be identical on both)
typedef struct {
  bool estop;        // true = E-Stop triggered
  bool heartbeat;    // always true, used to detect signal loss on RX side
} EStopMessage;

EStopMessage outMsg;
esp_now_peer_info_t peerInfo;
bool estopActive = false;

// Callback: fires after each send attempt
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("WARNING: Send failed!");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Button connects pin to GND when pressed
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);

  // Start WiFi in station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init FAILED. Halting.");
    while (true);  // Halt - do not run without comms
  }

  esp_now_register_send_cb(onSent);

  // Register receiver as a peer
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;      // 0 = auto channel
  peerInfo.encrypt = false;  // No encryption (add if needed for security)

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer. Check MAC address.");
    while (true);
  }

  Serial.println("TX Ready - Wireless E-Stop Armed");
  digitalWrite(LED_GREEN, HIGH);
}

void loop() {
  // Read button (LOW = pressed because INPUT_PULLUP)
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

  // Once E-Stop is triggered it LATCHES - requires physical reset for safety
  if (buttonPressed) {
    estopActive = true;
  }

  // Build message
  outMsg.estop     = estopActive;
  outMsg.heartbeat = true;

  // Send to receiver
  esp_now_send(receiverMAC, (uint8_t *)&outMsg, sizeof(outMsg));

  // Update indicator LEDs
  digitalWrite(LED_RED,   estopActive ? HIGH : LOW);
  digitalWrite(LED_GREEN, estopActive ? LOW  : HIGH);

  if (estopActive) {
    Serial.println("!!! E-STOP TRANSMITTED !!!");
  }

  delay(100);  // Transmit at 10Hz (every 100ms)
}