#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "legs.h"

#include <esp_now.h>
#include <WiFi.h>

// ================= ESP-NOW =================
volatile char command = 0;
volatile bool commandReady = false;

// callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len >= 1) {
    command = incomingData[0];
    commandReady = true;
  }
}


bool isInterrupted() {
  if (commandReady) {
    commandReady = false;
    return true;
  }
  return false;
}

// ================= ROBOT =================

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Quadruped robot(0, 1, 2, 3, 4, 5, 6, 7);

// Standing positions
const int STAND_FL_FIBULA = 180; const int STAND_FL_FEMUR = 135;
const int STAND_FR_FEMUR = 45;  const int STAND_FR_FIBULA = 0;
const int STAND_BL_FEMUR = 45;  const int STAND_BL_FIBULA = 0;
const int STAND_BR_FEMUR = 135; const int STAND_BR_FIBULA = 180;

struct JointAngles {
  int fl_femur = STAND_FL_FEMUR; int fl_fibula = STAND_FL_FIBULA;
  int bl_femur = STAND_BL_FEMUR; int bl_fibula = STAND_BL_FIBULA;
  int fr_femur = STAND_FR_FEMUR; int fr_fibula = STAND_FR_FIBULA;
  int br_femur = STAND_BR_FEMUR; int br_fibula = STAND_BR_FIBULA;
} joints;

// ================= CONTROL =================

void applyJointAngles() {
  robot.frontLeft.setTarget(joints.fl_femur, joints.fl_fibula);
  robot.frontRight.setTarget(joints.fr_femur, joints.fr_fibula);
  robot.backLeft.setTarget(joints.bl_femur, joints.bl_fibula);
  robot.backRight.setTarget(joints.br_femur, joints.br_fibula);
}

void applyJointAnglesSmoothly() {
  while ((robot.frontLeft.femur.currentAngle != joints.fl_femur) || (robot.frontLeft.fibula.currentAngle != joints.fl_fibula)
      || (robot.frontRight.femur.currentAngle != joints.fr_femur) || (robot.frontRight.fibula.currentAngle != joints.fr_fibula)
      || (robot.backLeft.femur.currentAngle != joints.bl_femur) || (robot.backLeft.fibula.currentAngle != joints.bl_fibula)
      || (robot.backRight.femur.currentAngle != joints.br_femur) || (robot.backRight.fibula.currentAngle != joints.br_fibula)) {
    robot.update();
    // if (isInterrupted) return;
  }
}

void standUp() {
  joints.fl_femur = STAND_FL_FEMUR; joints.fl_fibula = STAND_FL_FIBULA;
  joints.fr_femur = STAND_FR_FEMUR; joints.fr_fibula = STAND_FR_FIBULA;
  joints.bl_femur = STAND_BL_FEMUR; joints.bl_fibula = STAND_BL_FIBULA;
  joints.br_femur = STAND_BR_FEMUR; joints.br_fibula = STAND_BR_FIBULA;
  applyJointAngles();
}

// ================= ACTIONS =================

void jump() {

  joints.fl_fibula = 90;
  joints.fr_fibula = 90;
  joints.bl_fibula = 90;
  joints.br_fibula = 90;
  applyJointAngles();

  applyJointAnglesSmoothly();

  // for (int i = 0; i < 50; i++) robot.update();

  joints.fl_fibula = 180;
  joints.fr_fibula = 0;
  joints.bl_fibula = 0;
  joints.br_fibula = 180;
  applyJointAngles();

  // for (int i = 0; i < 50; i++) robot.update();
  applyJointAnglesSmoothly();
}

void walk () {
  
  // step 1: lift hands
  joints.fl_fibula = 45;
  joints.br_fibula = 135;
  applyJointAngles();
  applyJointAnglesSmoothly();
  

  // step 2 move femurs

  joints.fl_femur = 180; 
  joints.br_femur = 90; 

  applyJointAngles();
  applyJointAnglesSmoothly();

  // step 3: move other legs forward
  joints.bl_femur = 0;
  joints.bl_fibula = 45;
  joints.fl_fibula = 90;
  applyJointAngles();
  applyJointAnglesSmoothly();
  


  joints.fl_fibula = 90;
  joints.br_fibula = 180;
  applyJointAngles();
  applyJointAnglesSmoothly();
  

  

  standUp();


  joints.fr_fibula = 90;
  joints.bl_fibula = 90;
  applyJointAngles();
  applyJointAnglesSmoothly();
  

  joints.fr_femur = 0; 
  joints.bl_femur = 90; 

  applyJointAngles();
  applyJointAnglesSmoothly();
  joints.br_femur = 180;
  joints.br_fibula = 135;
  joints.fr_fibula = 0;
  applyJointAngles();
  applyJointAnglesSmoothly();

  joints.fr_fibula = 0;
  joints.bl_fibula = 0;
  applyJointAngles();
  applyJointAnglesSmoothly();

  

  standUp();
  

}

// ================= COMMAND HANDLER =================

void handleCommand(char key) {

  switch (key) {

    case 'q':
    case 'Q':
      standUp();
      Serial.println("Stand");
      break;

    case 'w':
    case 'W':
      walk();
      Serial.println("Walk");
      break;

    case ' ':
      jump();
      Serial.println("Jump");
      break;
  }
}

// ================= SETUP =================

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin();  // 🔥 REQUIRED FIX

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  pwm.begin();
  pwm.setPWMFreq(60);

  applyJointAngles();

  Serial.println("=== Robot Ready (Fixed ESP-NOW) ===");
}

// ================= LOOP =================

void loop() {

  if (commandReady) {
    commandReady = false;

    if (command >= 32) {
      handleCommand(command);
    }
  }

  robot.update();
}