#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <vector>

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6
#define MAX_MASTERS 10

/* Global Variables */

// List of all the masters MAC addresses
uint8_t master_mac[MAX_MASTERS][6];
int master_count = 0;

// Latest received message
char latest_message[250];
uint8_t latest_sender[6];
bool new_message_available = false;

/* Callbacks */

// Callback called when data is received
void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  // Store the message instantly
  int copy_len = (len < 249) ? len : 249;
  memcpy(latest_message, data, copy_len);
  latest_message[copy_len] = '\0';
  memcpy(latest_sender, mac_addr, 6);
  new_message_available = true;
  
  Serial.printf("Received from " MACSTR ": %s\n", MAC2STR(mac_addr), latest_message);
  
  // Check if this is a new master
  bool known = false;
  for (int i = 0; i < master_count; i++) {
    if (memcmp(mac_addr, master_mac[i], 6) == 0) {
      known = true;
      break;
    }
  }
  
  if (!known && master_count < MAX_MASTERS) {
    Serial.println("New master registered!");
    memcpy(master_mac[master_count], mac_addr, 6);
    master_count++;
    Serial.printf("Total masters: %d\n", master_count);
  }
}


void receive_init() {

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  
  // Set Wi-Fi channel for ESP-NOW
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.println("ESP-NOW Example - Broadcast Slave");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %u\n", ESPNOW_WIFI_CHANNEL);

  // Initialize the ESP-NOW protocol
  esp_err_t result = esp_now_init();
  if (result != ESP_OK) {
    Serial.printf("Failed to initialize ESP-NOW: %s\n", esp_err_to_name(result));
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Register the receive callback
  esp_now_register_recv_cb(onDataReceive);

  Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}



// Function to get the latest message (returns true if new message available)
bool get_latest_message(char *buffer, int buffer_size, uint8_t *sender_mac) {
  if (!new_message_available) {
    return false;
  }
  
  strncpy(buffer, latest_message, buffer_size - 1);
  buffer[buffer_size - 1] = '\0';
  
  if (sender_mac) {
    memcpy(sender_mac, latest_sender, 6);
  }
  
  new_message_available = false;
  return true;
}

// Function to peek at latest message without consuming it
bool peek_latest_message(char *buffer, int buffer_size, uint8_t *sender_mac) {
  if (!new_message_available) {
    return false;
  }
  
  strncpy(buffer, latest_message, buffer_size - 1);
  buffer[buffer_size - 1] = '\0';
  
  if (sender_mac) {
    memcpy(sender_mac, latest_sender, 6);
  }
  
  return true;
}

// Legacy function for compatibility
void receive_msg() {
  // Now messages are received instantly via callback
  // This function can be used for other periodic tasks if needed
  delay(10);
}


