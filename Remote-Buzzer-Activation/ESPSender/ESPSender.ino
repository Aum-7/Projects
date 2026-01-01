/*
    ESP-NOW Broadcast Master (Button Triggered)
    Sends a broadcast ESP-NOW message every time a button is pressed.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h> 

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6   // Wi-Fi channel used for ESP-NOW communication

// GPIO pin connected to the button.
// Using INPUT_PULLUP means the pin is HIGH normally and LOW when pressed.
#define BUTTON_PIN 10 

/* Classes */

// Custom class representing a broadcast-only ESP-NOW peer
class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  // Constructor: initializes peer using the broadcast MAC address
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  // Destructor: ensures peer is removed cleanly
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  // Initialize ESP-NOW and register the broadcast peer
  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  // Send a message to the broadcast address
  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      return false;
    }
    return true;
  }
};

/* Global Variables */

uint32_t msg_count = 0;       // Counts how many messages have been sent
int lastButtonState = HIGH;   // Tracks previous button state for edge detection

// Create a broadcast peer instance
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

/* Main */

void setup() {
  Serial.begin(115200);

  // Configure button pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configure Wi-Fi in station mode and set the ESP-NOW channel
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

  // Wait until Wi-Fi station mode is fully started
  while (!WiFi.STA.started()) {
    delay(100);
  }

  // Print basic system info
  Serial.println("ESP-NOW Example - Broadcast Master (Button Trigger)");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW broadcast peer
  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize broadcast peer");
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Print ESP-NOW capabilities
  Serial.printf("ESP-NOW version: %d, max data length: %d\n", ESP_NOW.getVersion(), ESP_NOW.getMaxDataLen());

  Serial.println("Setup complete. Press the button to broadcast a message.");
}

void loop() {
  // Read the current button state
  int currentButtonState = digitalRead(BUTTON_PIN);

  // Detect falling edge: HIGH → LOW means button was just pressed
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    
    // Create a message containing the press count
    char data[32];
    snprintf(data, sizeof(data), "Button Pressed! #%lu", ++msg_count);

    Serial.printf("Sending Broadcast: %s\n", data);

    // Send the ESP-NOW broadcast message
    if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) {
      Serial.println("Failed to broadcast message");
    }

    // Debounce delay to avoid multiple triggers from one press
    delay(250);
  }

  // Save current state for next loop iteration
  lastButtonState = currentButtonState;
}
