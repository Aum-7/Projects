/*
    ESP-NOW Broadcast Slave (Buzzer Trigger)
    - Listens for ESP-NOW broadcast messages from any master.
    - When a message is received, the buzzer turns ON for a fixed duration.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <vector>

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6   // Wi-Fi channel used for ESP-NOW communication

// Buzzer configuration
#define BUZZER_PIN 10           // GPIO pin connected to the buzzer
#define BUZZER_DURATION 1000    // Duration (ms) the buzzer stays ON after a message

/* Global Variables */

// Tracks when the buzzer was last activated
volatile unsigned long buzzer_timer = 0;

// Indicates whether the buzzer is currently ON
volatile bool buzzer_active = false;

// Dynamic list of all discovered master devices
std::vector<class ESP_NOW_Peer_Class *> masters;

/* Classes */

// Represents a master peer that sends ESP-NOW messages
class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor: initializes peer with MAC address and ESP-NOW settings
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  ~ESP_NOW_Peer_Class() {}

  // Register this peer with ESP-NOW
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Called automatically when a message is received from this peer
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    Serial.printf("Received a message from master " MACSTR " (%s)\n",
                  MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    Serial.printf("  Message: %s\n", (char *)data);

    // Activate buzzer when a message arrives
    digitalWrite(BUZZER_PIN, HIGH);
    buzzer_timer = millis();   // Start/reset timer
    buzzer_active = true;      // Mark buzzer as active
  }
};

/* Callbacks */

// Called when a new peer sends a broadcast message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {

  // Check if the message was broadcast (not unicast)
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    // Create a new master peer object
    ESP_NOW_Peer_Class *new_master =
      new ESP_NOW_Peer_Class(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

    // Attempt to register the peer with ESP-NOW
    if (!new_master->add_peer()) {
      Serial.println("Failed to register the new master");
      delete new_master;
      return;
    }

    // Store the new master in the list
    masters.push_back(new_master);

    Serial.printf("Successfully registered master " MACSTR " (total masters: %zu)\n",
                  MAC2STR(new_master->addr()), masters.size());

    // Trigger buzzer for the first message from a new master
    digitalWrite(BUZZER_PIN, HIGH);
    buzzer_timer = millis();
    buzzer_active = true;

  } else {
    // Ignore unicast messages (not relevant for broadcast discovery)
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Ignoring the message");
  }
}

/* Main */

void setup() {
  Serial.begin(115200);

  // Initialize buzzer pin as output and ensure it starts OFF
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Configure Wi-Fi in station mode and set ESP-NOW channel
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

  // Wait until Wi-Fi is fully initialized
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("ESP-NOW - Broadcast Slave (Buzzer Trigger)");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  Serial.printf("ESP-NOW version: %d, max data length: %d\n",
                ESP_NOW.getVersion(), ESP_NOW.getMaxDataLen());

  // Register callback for discovering new masters
  ESP_NOW.onNewPeer(register_new_master, nullptr);

  Serial.println("Setup complete. Waiting for signal to turn on Buzzer...");
}

void loop() {
  // Non-blocking buzzer timer:
  // Turn OFF buzzer after BUZZER_DURATION milliseconds
  if (buzzer_active && (millis() - buzzer_timer > BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzer_active = false;
    Serial.println("Buzzer turned OFF");
  }

  // Print debug info every 10 seconds
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 10000) {
    last_debug = millis();
    Serial.printf("Registered masters: %zu\n", masters.size());
  }

  // Small delay for stability
  delay(10);
}
