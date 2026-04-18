/*
  WIRELESS E-STOP - TRANSMITTER (TX)
  Behavior : Sends heartbeat and an incrementing counter every 100ms.
  If E-Stop is triggered, counter resets to 0.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // Added for Long Range mode

// -------------------------------------------------------
// Your Receiver's MAC Address
// -------------------------------------------------------
uint8_t receiverMAC[] = {0x1C, 0xDB, 0xD4, 0xAE, 0xD5, 0x88};
#define BUTTON_PIN          4   // E-Stop button pin (momentary NO button, pulls LOW when pressed)
#define HARDWARE_ESTOP_PIN  16  // Connect to 3.3V to trigger E-Stop, leave floating/GND for normal
#define LED_GREEN           25  // Green = system running
#define LED_RED             26  // Red   = E-Stop active

// Message structure shared by TX and RX (must be identical on both)
typedef struct {
  bool estop;
  bool heartbeat;
  bool hwEstop;
  long counter;    // Changed to long so it can count very high
} EStopMessage;

EStopMessage outMsg;
esp_now_peer_info_t peerInfo;
bool estopActive = false;
bool hwEstopActive = false;

// The continuous counter
long sequenceNumber = 1;

// Callback for ESP32 Core v3.x
void onSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("WARNING: Send failed!");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN,         INPUT_PULLUP);
  pinMode(HARDWARE_ESTOP_PIN, INPUT);
  pinMode(LED_GREEN,          OUTPUT);
  pinMode(LED_RED,            OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // --- NEW ADDITIONS FOR MAXIMUM RANGE ---
  // Enable ESP32 Long Range (LR) Protocol for 128kbps/256kbps sensitivity
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  // Force Maximum Transmit Power (19.5 dBm)
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  // ---------------------------------------

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init FAILED. Halting.");
    while (true);
  }

  esp_now_register_send_cb(onSent);

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer. Check MAC address.");
    while (true);
  }

  Serial.println("TX Ready - LR Mode Enabled - Armed at MAX TX Power");
  digitalWrite(LED_GREEN, HIGH);
}

void loop() {
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  bool hwPinTriggered = (digitalRead(HARDWARE_ESTOP_PIN) == HIGH);

  // Latch E-Stop
  if (buttonPressed) {
    estopActive = true;
  }
  if (hwPinTriggered) {
    estopActive   = true;
    hwEstopActive = true;
  }

  // -------------------------------------------------------
  // COUNTER LOGIC
  // -------------------------------------------------------
  if (estopActive) {
    sequenceNumber = 0;
    // Reset counter to zero when E-Stop is triggered
  }

  // Build and send message
  outMsg.estop     = estopActive;
  outMsg.heartbeat = true;
  outMsg.hwEstop   = hwEstopActive;
  outMsg.counter   = sequenceNumber;

  // Print to Serial Monitor
  Serial.print("TX | E-Stop State: ");
  Serial.print(estopActive ? "TRIGGERED" : "RUNNING  ");
  Serial.print(" | Sending Counter: ");
  Serial.println(outMsg.counter);

  esp_now_send(receiverMAC, (uint8_t *)&outMsg, sizeof(outMsg));

  // Increment counter ONLY if E-stop is NOT active
  if (!estopActive) {
    sequenceNumber++;
  }

  // Update LEDs
  digitalWrite(LED_RED,   estopActive ? HIGH : LOW);
  digitalWrite(LED_GREEN, estopActive ? LOW  : HIGH);

  delay(100); // 10 packets per second
}