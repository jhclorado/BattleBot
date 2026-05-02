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
Quadruped robot(8, 1, 2, 3, 4, 5, 6, 7);
Controller controller(&robot);

// Manual control settings
const int JOINT_STEP = 5;  // degrees per keypress
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

// Current joint angles for manual control
struct JointAngles {
  int fl_femur = 150;
  int fl_fibula = 50;
  int fr_femur = 35;
  int fr_fibula = 0;
  int bl_femur = 90;
  int bl_fibula = 35;
  int br_femur = 0;
  int br_fibula = 100;
} joints;

bool autoWalk = false;
unsigned long autoWalkLastStep = 0;
const unsigned long AUTO_WALK_STEP_MS = 500;
int autoWalkPhase = 0;

void applyJointAngles() {
  robot.frontLeft.setTarget(joints.fl_femur, joints.fl_fibula);
  robot.frontRight.setTarget(joints.fr_femur, joints.fr_fibula);
  robot.backLeft.setTarget(joints.bl_femur, joints.bl_fibula);
  robot.backRight.setTarget(joints.br_femur, joints.br_fibula);
}

void startAutoWalk() {
  delay(2000);
  Serial.println("Starting uncontrolled test walk...");
  autoWalk = true;
  autoWalkLastStep = millis();
  autoWalkPhase = 0;
}

void runAutoWalkStep() {
  if (!autoWalk) {
    return;
  }

  unsigned long now = millis();
  if (now - autoWalkLastStep < AUTO_WALK_STEP_MS) {
    return;
  }
  autoWalkLastStep = now;

  switch (autoWalkPhase) {
    case 0:
      robot.frontLeft.setTarget(130, 40);
      robot.backRight.setTarget(130, 40);
      robot.frontRight.setTarget(50, 80);
      robot.backLeft.setTarget(50, 80);
      break;

    case 1:
      robot.frontLeft.setTarget(100, 60);
      robot.backRight.setTarget(100, 60);
      robot.frontRight.setTarget(80, 60);
      robot.backLeft.setTarget(80, 60);
      break;

    case 2:
      robot.frontLeft.setTarget(70, 80);
      robot.backRight.setTarget(70, 80);
      robot.frontRight.setTarget(110, 40);
      robot.backLeft.setTarget(110, 40);
      break;

    case 3:
      robot.frontLeft.setTarget(90, 60);
      robot.backRight.setTarget(90, 60);
      robot.frontRight.setTarget(90, 60);
      robot.backLeft.setTarget(90, 60);
      break;
  }

  autoWalkPhase = (autoWalkPhase + 1) % 4;
}

void handleSerialInput(char key) {
  bool updated = false;
  
  switch(key) {
    // Front Left Femur
    case 'q':
    case 'Q':
    case '1':  // Front Left Femur +
      joints.fl_femur = min(joints.fl_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FL Femur: %d\n", joints.fl_femur);
      break;
    case 'a':
    case 'A':
    case '!':  // Front Left Femur -
      joints.fl_femur = max(joints.fl_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FL Femur: %d\n", joints.fl_femur);
      break;
    
    // Front Left Fibula
    case 'w':
    case 'W':
    case '2':  // Front Left Fibula +
      joints.fl_fibula = min(joints.fl_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FL Fibula: %d\n", joints.fl_fibula);
      break;
    case 's':
    case 'S':
    case '@':  // Front Left Fibula -
      joints.fl_fibula = max(joints.fl_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FL Fibula: %d\n", joints.fl_fibula);
      break;
    
    // Front Right Femur
    case 'e':
    case 'E':
    case '3':  // Front Right Femur +
      joints.fr_femur = min(joints.fr_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FR Femur: %d\n", joints.fr_femur);
      break;
    case 'd':
    case 'D':
    case '#':  // Front Right Femur -
      joints.fr_femur = max(joints.fr_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FR Femur: %d\n", joints.fr_femur);
      break;
    
    // Front Right Fibula
    case 'r':
    case 'R':
    case '4':  // Front Right Fibula +
      joints.fr_fibula = min(joints.fr_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FR Fibula: %d\n", joints.fr_fibula);
      break;
    case 'f':
    case 'F':
    case '$':  // Front Right Fibula -
      joints.fr_fibula = max(joints.fr_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FR Fibula: %d\n", joints.fr_fibula);
      break;
    
    // Back Left Femur
    case 'z':
    case 'Z':
    case '5':  // Back Left Femur +
      joints.bl_femur = min(joints.bl_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BL Femur: %d\n", joints.bl_femur);
      break;
    case 'x':
    case 'X':
    case '%':  // Back Left Femur -
      joints.bl_femur = max(joints.bl_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BL Femur: %d\n", joints.bl_femur);
      break;
    
    // Back Left Fibula
    case 'c':
    case 'C':
    case '6':  // Back Left Fibula +
      joints.bl_fibula = min(joints.bl_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BL Fibula: %d\n", joints.bl_fibula);
      break;
    case 'v':
    case 'V':
    case '^':  // Back Left Fibula -
      joints.bl_fibula = max(joints.bl_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BL Fibula: %d\n", joints.bl_fibula);
      break;
    
    // Back Right Femur
    case 't':
    case 'T':
    case '7':  // Back Right Femur +
      joints.br_femur = min(joints.br_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BR Femur: %d\n", joints.br_femur);
      break;
    case 'g':
    case 'G':
    case '&':  // Back Right Femur -
      joints.br_femur = max(joints.br_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BR Femur: %d\n", joints.br_femur);
      break;
    
    // Back Right Fibula
    case 'y':
    case 'Y':
    case '8':  // Back Right Fibula +
      joints.br_fibula = min(joints.br_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BR Fibula: %d\n", joints.br_fibula);
      break;
    case 'h':
    case 'H':
    case '*':  // Back Right Fibula -
      joints.br_fibula = max(joints.br_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BR Fibula: %d\n", joints.br_fibula);
      break;
    
    case '?':  // Help
      Serial.println("\n=== Manual Control ===");
      Serial.println("Each joint has 3 key options:");
      Serial.println("\nFront Left:  q/1(+) a/!(−) femur | w/2(+) s/@(−) fibula");
      Serial.println("Front Right: e/3(+) d/#(−) femur | r/4(+) f/$(−) fibula");
      Serial.println("Back Left:   z/5(+) x/%(−) femur | c/6(+) v/^(−) fibula");
      Serial.println("Back Right:  t/7(+) g/&(−) femur | y/8(+) h/*(−) fibula");
      Serial.println("\n? = Help\n");
      break;
  }
  
  if (updated) {
    applyJointAngles();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);  // Wait for serial to stabilize
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(60);
  
  // Initialize joint angles
  applyJointAngles();
  
  // Initialize ESP-NOW receiver
  receive_init();
  
  Serial.println("\n=== BattleBot Ready ===");
  Serial.println("Type '?' for control help");

  startAutoWalk();
}

void loop() {
  if (autoWalk) {
    runAutoWalkStep();
  } else {
    // Check for serial input
    if (Serial.available()) {
      char key = Serial.read();
      if (key >= 32) {  // Only process printable characters
        handleSerialInput(key);
      }
    }
    
    // Check for new ESP-NOW messages
    char msg[250];
    uint8_t sender[6];
    if (get_latest_message(msg, sizeof(msg), sender)) {
      controller.handlePacket((uint8_t*)msg, strlen(msg));
    }
  }
  
  robot.update();
}
