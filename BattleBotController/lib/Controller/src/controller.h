#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_mac.h>

// ESP-NOW channel (must match robot)
#define ESPNOW_WIFI_CHANNEL 6

// Robot servo positions
struct RobotPose {
  int frontLeft[2];   // femur, fibula
  int frontRight[2];
  int backLeft[2];
  int backRight[2];
};

class RobotController {
private:
  // Current pose
  RobotPose currentPose = {{90, 45}, {90, 45}, {90, 45}, {90, 45}};

  // ESP-NOW peer info (broadcast to all)
  esp_now_peer_info_t peerInfo;

  // Sequence control
  bool sequenceActive = false;
  unsigned long sequenceStartTime = 0;
  int sequenceStep = 0;

  // Random mode control
  bool randomModeActive = false;
  unsigned long lastRandomUpdateTime = 0;
  int randomUpdateInterval = 500; // Update random pose every 500ms

  // Send message via ESP-NOW broadcast
  void sendMessage(const String& message) {
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)message.c_str(), message.length());

    if (result == ESP_OK) {
      Serial.println("Sent: " + message);
    } else {
      Serial.println("Send failed: " + String(esp_err_to_name(result)));
    }
  }

  // Format pose as message string
  String poseToMessage(const RobotPose& pose) {
    char buffer[100];
    sprintf(buffer, "FL:%d,%d;FR:%d,%d;BL:%d,%d;BR:%d,%d",
            pose.frontLeft[0], pose.frontLeft[1],
            pose.frontRight[0], pose.frontRight[1],
            pose.backLeft[0], pose.backLeft[1],
            pose.backRight[0], pose.backRight[1]);
    return String(buffer);
  }

  // Send current pose
  void sendCurrentPose() {
    String message = poseToMessage(currentPose);
    sendMessage(message);
  }

  // Set pose and send
  void setPose(const RobotPose& pose) {
    currentPose = pose;
    sendCurrentPose();
  }

public:
  void init() {
    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("ESP-NOW init failed");
      ESP.restart();
    }

    // Register send callback
    esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status) {
      // Optional: handle send status
    });

    Serial.println("Controller initialized");
    Serial.println("MAC: " + WiFi.macAddress());
  }

  void processCommand(String command) {
    command.trim();
    command.toLowerCase();

    if (command.startsWith("fl:") || command.startsWith("fr:") || command.startsWith("bl:") || command.startsWith("br:")) {
      // Direct angle command
      parseAndSendAngles(command);
    } else if (command == "stand") {
      stand();
    } else if (command == "sit") {
      sit();
    } else if (command == "walk") {
      startWalkSequence();
    } else if (command == "stop") {
      stopSequence();
      stopRandomMode();
      // Send current pose to stop movement
      sendCurrentPose();
    } else if (command == "random") {
      startRandomMode();
    } else {
      Serial.println("Unknown command: " + command);
    }
  }

  void parseAndSendAngles(String cmd) {
    // Parse format: FL:90,45;FR:90,45;BL:90,45;BR:90,45
    RobotPose pose = {{90, 45}, {90, 45}, {90, 45}, {90, 45}}; // defaults

    int fl1, fl2, fr1, fr2, bl1, bl2, br1, br2;
    int parsed = sscanf(cmd.c_str(),
                       "fl:%d,%d;fr:%d,%d;bl:%d,%d;br:%d,%d",
                       &fl1, &fl2, &fr1, &fr2, &bl1, &bl2, &br1, &br2);

    if (parsed >= 8) {
      pose.frontLeft[0] = fl1; pose.frontLeft[1] = fl2;
      pose.frontRight[0] = fr1; pose.frontRight[1] = fr2;
      pose.backLeft[0] = bl1; pose.backLeft[1] = bl2;
      pose.backRight[0] = br1; pose.backRight[1] = br2;
      setPose(pose);
    } else {
      Serial.println("Invalid angle format. Use: FL:90,45;FR:90,45;BL:90,45;BR:90,45");
    }
  }

  void stand() {
    RobotPose standPose = {{90, 45}, {90, 45}, {90, 45}, {90, 45}};
    setPose(standPose);
    Serial.println("Standing position");
  }

  void sit() {
    RobotPose sitPose = {{45, 90}, {45, 90}, {135, 90}, {135, 90}};
    setPose(sitPose);
    Serial.println("Sitting position");
  }

  void startWalkSequence() {
    sequenceActive = true;
    sequenceStartTime = millis();
    sequenceStep = 0;
    Serial.println("Starting walk sequence");
  }

  void stopSequence() {
    sequenceActive = false;
    Serial.println("Sequence stopped");
  }

  void startRandomMode() {
    randomModeActive = true;
    sequenceActive = false;
    lastRandomUpdateTime = millis();
    Serial.println("Random mode started - sending random control signals every " + String(randomUpdateInterval) + "ms");
  }

  void stopRandomMode() {
    randomModeActive = false;
    Serial.println("Random mode stopped");
  }

  void update() {
    if (randomModeActive) {
      unsigned long now = millis();
      if (now - lastRandomUpdateTime >= randomUpdateInterval) {
        lastRandomUpdateTime = now;
        RobotPose randomPose = generateRandomPose();
        setPose(randomPose);
      }
    } else if (sequenceActive) {
      unsigned long now = millis();
      unsigned long elapsed = now - sequenceStartTime;

      // Simple walk cycle: 4 steps per cycle, 1 second each
      int cycleTime = 1000; // 1 second per step
      int stepInCycle = (elapsed / cycleTime) % 4;

      if (stepInCycle != sequenceStep) {
        sequenceStep = stepInCycle;
        executeWalkStep(sequenceStep);
      }
    }
  }

  void executeWalkStep(int step) {
    RobotPose walkPose;

    switch (step) {
      case 0: // Lift front left, move back right forward
        walkPose = {{135, 90}, {90, 45}, {90, 45}, {45, 0}};
        break;
      case 1: // Lift front right, move back left forward
        walkPose = {{90, 45}, {135, 90}, {45, 0}, {90, 45}};
        break;
      case 2: // Lift back left, move front right forward
        walkPose = {{45, 0}, {90, 45}, {135, 90}, {90, 45}};
        break;
      case 3: // Lift back right, move front left forward
        walkPose = {{90, 45}, {45, 0}, {90, 45}, {135, 90}};
        break;
    }

    setPose(walkPose);
    Serial.printf("Walk step %d\n", step + 1);
  }

  // Generate random servo angles (constrained between 0-180 degrees)
  RobotPose generateRandomPose() {
    RobotPose randomPose;
    randomPose.frontLeft[0] = random(0, 181);
    randomPose.frontLeft[1] = random(0, 181);
    randomPose.frontRight[0] = random(0, 181);
    randomPose.frontRight[1] = random(0, 181);
    randomPose.backLeft[0] = random(0, 181);
    randomPose.backLeft[1] = random(0, 181);
    randomPose.backRight[0] = random(0, 181);
    randomPose.backRight[1] = random(0, 181);
    return randomPose;
  }
};

#endif