#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "legs.h"
#include "comm.h"
#include "espnow.h"

// Initialize PCA9685 PWM controller
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Quadruped with 8 servos (2 per limb)
// Servo indices: FL_femur=0, FL_fibula=1, FR_femur=2, FR_fibula=3, 
//                BL_femur=4, BL_fibula=5, BR_femur=6, BR_fibula=7
Quadruped robot(0, 1, 2, 3, 4, 5, 6, 7);
Controller controller(&robot);

void setup() {
  Serial.begin(115200);
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(60);
  
  // Initialize ESP-NOW receiver
  receive_init();
  
  Serial.println("BattleBot Ready");
}

void loop() {
  // Check for new ESP-NOW messages
  char msg[250];
  uint8_t sender[6];
  if (get_latest_message(msg, sizeof(msg), sender)) {
    controller.handlePacket((uint8_t*)msg, strlen(msg));
  }
  
  robot.update();
}
