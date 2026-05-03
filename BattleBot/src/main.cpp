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
Quadruped robot(8, 9, 2, 3, 4, 5, 6, 7);
Controller controller(&robot);

// Manual control settings
const int JOINT_STEP = 5;  // degrees per keypress
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

// Current joint angles for manual control
struct JointAngles {
  int fl_femur = 130;
  int fl_fibula = 90;
  int fr_femur = 45;
  int fr_fibula = 0;
  int bl_femur = 35;
  int bl_fibula = 0;
  int br_femur =135;
  int br_fibula = 180;
} joints;


void applyJointAngles() {
  robot.frontLeft.setTarget(joints.fl_femur, joints.fl_fibula);
  robot.frontRight.setTarget(joints.fr_femur, joints.fr_fibula);
  robot.backLeft.setTarget(joints.bl_femur, joints.bl_fibula);
  robot.backRight.setTarget(joints.br_femur, joints.br_fibula);
}

void standUp() {
  joints.fl_femur = 130; joints.fl_fibula = 90;
  joints.fr_femur = 45;  joints.fr_fibula = 0;
  joints.bl_femur = 35;  joints.bl_fibula = 0;
  joints.br_femur =135;  joints.br_fibula = 180;
  applyJointAngles();
}

void jump () {
  // step 1: squat
  joints.fl_fibula = 0;
  joints.bl_fibula = 90;
  joints.br_fibula = 90;
  joints.fr_fibula = 90;

  applyJointAngles();
  while ((robot.frontLeft.fibula.currentAngle != joints.fl_fibula) ||
         (robot.backLeft.fibula.currentAngle != joints.bl_fibula) ||
         (robot.backRight.fibula.currentAngle != joints.br_fibula) ||
         (robot.frontRight.fibula.currentAngle != joints.fr_fibula) ) {
    robot.update();
         }
  
  // step 1: stand up quickly
  joints.fl_fibula = 90;
  joints.bl_fibula = 0;
  joints.br_fibula = 180;
  joints.fr_fibula = 0;
  applyJointAngles();
  while ((robot.frontLeft.fibula.currentAngle != joints.fl_fibula) ||
         (robot.backLeft.fibula.currentAngle != joints.bl_fibula) ||
         (robot.backRight.fibula.currentAngle != joints.br_fibula) ||
         (robot.frontRight.fibula.currentAngle != joints.fr_fibula) ) {
    robot.update();
         }

}

bool isInterrupted() {
  if (Serial.available()) {
      char key = Serial.read();
      if (key >= 32) {  // Only process printable characters
        if (key != 'w' && key != 'W') {  // Don't interrupt walk command
          standUp();
          return true;
        }
      }
    }
  return false;
}

void walk () {
  
  // step 1: lift hands
  joints.fl_fibula = 45;
  joints.br_fibula = 135;
  applyJointAngles();
  while ((robot.frontLeft.fibula.currentAngle != joints.fl_fibula) || (robot.backRight.fibula.currentAngle != joints.br_fibula)) {
    robot.update();
    if (isInterrupted()) return;
  }

  // step 2 move femurs

  joints.fl_femur = 180; 
  joints.br_femur = 90; 

  applyJointAngles();
  while ((robot.frontLeft.femur.currentAngle != joints.fl_femur) ||
         (robot.backRight.femur.currentAngle != joints.br_femur) ) {
    robot.update();
    if (isInterrupted()) return;
  }
  
  // step 3: move other legs forward
  joints.bl_femur = 0;
  joints.bl_fibula = 45;
  joints.fl_fibula = 90;
  applyJointAngles();
  while (robot.backLeft.femur.currentAngle != joints.bl_femur || robot.backLeft.fibula.currentAngle != joints.bl_fibula) {
    robot.update();
    if (isInterrupted()) return;
  }


  joints.fl_fibula = 90;
  joints.br_fibula = 180;
  applyJointAngles();
  while ((robot.frontLeft.fibula.currentAngle != joints.fl_fibula) || (robot.backRight.fibula.currentAngle != joints.br_fibula)) {
    robot.update();
    if (isInterrupted()) return;
  }

  

  standUp();


  joints.fr_fibula = 45;
  joints.bl_fibula = 45;
  applyJointAngles();
  while ((robot.frontRight.fibula.currentAngle != joints.fr_fibula) || (robot.backLeft.fibula.currentAngle != joints.bl_fibula)) {
    robot.update();
    if (isInterrupted()) return;
  }

  joints.fr_femur = 0; 
  joints.bl_femur = 90; 

  applyJointAngles();
  while ((robot.frontRight.femur.currentAngle != joints.fr_femur) ||
         (robot.backLeft.femur.currentAngle != joints.bl_femur) ) {
    robot.update();
    if (isInterrupted()) return;
  }

  joints.br_femur = 180;
  joints.br_fibula = 135;
  joints.fr_fibula = 0;
  applyJointAngles();
  while (robot.backRight.femur.currentAngle != joints.br_femur || robot.backRight.fibula.currentAngle != joints.br_fibula) {
    robot.update();
    if (isInterrupted()) return;
  }

  joints.fr_fibula = 0;
  joints.bl_fibula = 0;
  applyJointAngles();
  while ((robot.frontRight.fibula.currentAngle != joints.fr_fibula) || (robot.backLeft.fibula.currentAngle != joints.bl_fibula)) {
    robot.update();
    if (isInterrupted()) return;
  }

  

  standUp();
  

}


// void walk() {
//   joints.br_femur = 135; joints.br_fibula = 130;
//   applyJointAngles();
//   Serial.println("Moving Back Right leg...");
//   while ((robot.backRight.femur.currentAngle != joints.br_femur) || (robot.backRight.fibula.currentAngle != joints.br_fibula)) {
//     robot.update();
//   }

//   joints.bl_femur = 100;  joints.bl_fibula = 0;
//   applyJointAngles();
//   while ((robot.backLeft.femur.currentAngle != joints.bl_femur) || (robot.backLeft.fibula.currentAngle != joints.bl_fibula)) {
//     robot.update();
//   }

//   joints.fr_femur = 45;  joints.fr_fibula = 10;
//   applyJointAngles();
//   while ((robot.frontRight.femur.currentAngle != joints.fr_femur) || (robot.frontRight.fibula.currentAngle != joints.fr_fibula)) {
//     robot.update();
//   }

//   joints.fl_femur = 130; joints.fl_fibula = 40;
//   applyJointAngles();
//   while ((robot.frontLeft.femur.currentAngle != joints.fl_femur) || (robot.frontLeft.fibula.currentAngle != joints.fl_fibula)) {
//     robot.update();
//   }


//   joints.br_femur = 135; joints.br_fibula = 180;
//   applyJointAngles();
//   Serial.println("Moving Back Right leg...");
//   while ((robot.backRight.femur.currentAngle != joints.br_femur) || (robot.backRight.fibula.currentAngle != joints.br_fibula)) {
//     robot.update();
//   }

// }

void handleSerialInput(char key) {
  bool updated = false;
  
  switch(key) {
    // Front Left Femur
    case '1':  // Front Left Femur +
      joints.fl_femur = min(joints.fl_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FL Femur: %d\n", joints.fl_femur);
      break;
    case '!':  // Front Left Femur -
      joints.fl_femur = max(joints.fl_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FL Femur: %d\n", joints.fl_femur);
      break;
    
    // Front Left Fibula
    case '2':  // Front Left Fibula +
      joints.fl_fibula = min(joints.fl_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FL Fibula: %d\n", joints.fl_fibula);
      break;

    case '@':  // Front Left Fibula -
      joints.fl_fibula = max(joints.fl_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FL Fibula: %d\n", joints.fl_fibula);
      break;
    
    // Front Right Femur
    case '3':  // Front Right Femur +
      joints.fr_femur = min(joints.fr_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FR Femur: %d\n", joints.fr_femur);
      break;

    case '#':  // Front Right Femur -
      joints.fr_femur = max(joints.fr_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FR Femur: %d\n", joints.fr_femur);
      break;
    
    // Front Right Fibula
    case '4':  // Front Right Fibula +
      joints.fr_fibula = min(joints.fr_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("FR Fibula: %d\n", joints.fr_fibula);
      break;
  
    case '$':  // Front Right Fibula -
      joints.fr_fibula = max(joints.fr_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("FR Fibula: %d\n", joints.fr_fibula);
      break;
    
    // Back Left Femur
    case '5':  // Back Left Femur +
      joints.bl_femur = min(joints.bl_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BL Femur: %d\n", joints.bl_femur);
      break;

    case '%':  // Back Left Femur -
      joints.bl_femur = max(joints.bl_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BL Femur: %d\n", joints.bl_femur);
      break;
    
    // Back Left Fibula
    case '6':  // Back Left Fibula +
      joints.bl_fibula = min(joints.bl_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BL Fibula: %d\n", joints.bl_fibula);
      break;
      
    case '^':  // Back Left Fibula -
      joints.bl_fibula = max(joints.bl_fibula - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BL Fibula: %d\n", joints.bl_fibula);
      break;
    
    // Back Right Femur
    case '7':  // Back Right Femur +
      joints.br_femur = min(joints.br_femur + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BR Femur: %d\n", joints.br_femur);
      break;
      
    case '&':  // Back Right Femur -
      joints.br_femur = max(joints.br_femur - JOINT_STEP, MIN_ANGLE);
      updated = true;
      Serial.printf("BR Femur: %d\n", joints.br_femur);
      break;
    
    // Back Right Fibula
    case '8':  // Back Right Fibula +
      joints.br_fibula = min(joints.br_fibula + JOINT_STEP, MAX_ANGLE);
      updated = true;
      Serial.printf("BR Fibula: %d\n", joints.br_fibula);
      break;
      
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

    case 'q':
    case 'Q':
      standUp();
      Serial.println("Standing up...");
      break;

    case 'w':
    case 'W':
      walk();
      Serial.println("Walking...");
      break;
    case ' ':
      jump();
      Serial.println("Jumping...");
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
}


void loop() {
  // Check for serial input
  if (Serial.available()) {
    char key = Serial.read();
    if (key >= 32) {  // Only process printable characters
      handleSerialInput(key);
    }
  }
  // walk();
  
  // Check for new ESP-NOW messages
  char msg[250];
  uint8_t sender[6];
  if (get_latest_message(msg, sizeof(msg), sender)) {
    controller.handlePacket((uint8_t*)msg, strlen(msg));
  }
  
  robot.update();
}

// #include <Wire.h>
// #include <Adafruit_PWMServoDriver.h>

// // Define custom I2C pins for the ESP32-CAM
// #define I2C_SDA 21
// #define I2C_SCL 22

// // Initialize the PCA9685 object
// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// // Servo parameters
// #define SERVOMIN 150  // Minimum pulse length for 0 degrees
// #define SERVOMAX 600  // Maximum pulse length for 180 degrees

// void setup() {
//   // Initialize Serial Monitor
//   Serial.begin(115200);
//   delay(2000); // Allow time for Serial Monitor to connect
//   Serial.println("Starting...");

//   // Initialize I2C with custom SDA and SCL pins
//   if (Wire.begin(I2C_SDA, I2C_SCL)) {
//     Serial.println("I2C initialized successfully.");
//   } else {
//     Serial.println("I2C initialization failed!");
//     while (true); // Stop execution if I2C fails
//   }

//   // Initialize PCA9685
//   pwm.begin();
//   pwm.setPWMFreq(50); // Set frequency to 50 Hz for servos
//   Serial.println("PCA9685 initialized.");
// }

// void loop() {
//   Serial.println("Moving servo on channel 1...");

//   // Sweep the servo on channel 1 from 0 to 180 degrees
//   for (int pulse = SERVOMIN; pulse <= SERVOMAX; pulse++) {
//     pwm.setPWM(1, 0, pulse); // Move servo on channel 1
//     delay(10); // Delay for smooth motion
//   }

//   // Sweep the servo back from 180 to 0 degrees
//   for (int pulse = SERVOMAX; pulse >= SERVOMIN; pulse--) {
//     pwm.setPWM(1, 0, pulse);
//     delay(10);
//   }

//   delay(1000); // Wait a bit before repeating
// }
