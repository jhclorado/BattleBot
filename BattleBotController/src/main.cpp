#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include "controller.h"

// Controller instance
RobotController controller;

void setup() {
  Serial.begin(115200);

  // Initialize controller (ESP-NOW broadcaster)
  controller.init();

  Serial.println("BattleBot Controller Ready");
  Serial.println("Commands:");
  Serial.println("  FL:ang1,ang2;FR:ang1,ang2;BL:ang1,ang2;BR:ang1,ang2 - Set angles");
  Serial.println("  stand - Stand up position");
  Serial.println("  sit - Sit down position");
  Serial.println("  walk - Basic walk cycle");
  Serial.println("  random - Send random control signals (TEST MODE)");
  Serial.println("  stop - Stop all movement");
  
  // Auto-start random mode for testing
  Serial.println("\nStarting automated testing mode...");
  controller.startRandomMode();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0) {
      controller.processCommand(command);
    }
  }

  // Update controller (handle any ongoing sequences)
  controller.update();
}