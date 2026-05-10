#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "legs.h"
#include "comm.h"
#include "espnow.h"


// ESP32 Mac Adress 30:76:f5:92:30:a8

// Initialize PCA9685 PWM controller
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Quadruped with 8 servos (2 per limb)
// Servo indices: FL_femur=0, FL_fibula=1, FR_femur=2, FR_fibula=3, 
//                BL_femur=4, BL_fibula=5, BR_femur=6, BR_fibula=7
Quadruped robot(0, 1, 2, 3, 4, 5, 6, 7);
Controller controller(&robot);

// Manual control settings
const int JOINT_STEP = 5;  // degrees per keypress
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

// Current joint angles for manual control

bool isTurningLeft = false; bool isTurningRight = false;
bool isWalking = false; bool isReverseWalking = false;
// 

// initial values for standing position
const int STAND_FL_FIBULA = 165; const int STAND_FL_FEMUR = 135;
const int STAND_FR_FEMUR = 45; const int STAND_FR_FIBULA = 15;
const int STAND_BL_FEMUR = 45; const int STAND_BL_FIBULA = 15;
const int STAND_BR_FEMUR = 135; const int STAND_BR_FIBULA = 165;

const int LIFT_FL_FIBULA = 115; const int LIFT_FR_FIBULA = 75;
const int LIFT_BL_FIBULA = 75; const int LIFT_BR_FIBULA = 115;


const int FL_X = 90; const int FL_Y = 180;
const int FR_X = 90; const int FR_Y = 0;
const int BL_X = 90; const int BL_Y = 0;
const int BR_X = 90; const int BR_Y = 180;

const int FL_XH = 90; const int FL_YH = 180;
const int FR_XH = 90; const int FR_YH = 0;
const int BL_XH = 90; const int BL_YH = 0;
const int BR_XH = 90; const int BR_YH = 180;

/** 
 * front left femur: 0 degrees is fully forward and 180 degrees is full leftward
 * front left fibula: 
 * 
 */

struct JointAngles {
  int fl_femur = STAND_FL_FEMUR; int fl_fibula = STAND_FL_FIBULA;
  int bl_femur = STAND_BL_FEMUR; int bl_fibula = STAND_BL_FIBULA;
  int fr_femur = STAND_FR_FEMUR; int fr_fibula = STAND_FR_FIBULA;
  int br_femur = STAND_BR_FEMUR; int br_fibula = STAND_BR_FIBULA;
} joints;


void applyJointAngles() {
  robot.frontLeft.setTarget(joints.fl_femur, joints.fl_fibula);
  robot.frontRight.setTarget(joints.fr_femur, joints.fr_fibula);
  robot.backLeft.setTarget(joints.bl_femur, joints.bl_fibula);
  robot.backRight.setTarget(joints.br_femur, joints.br_fibula);
}



void standUp() {
  isTurningLeft = false; isTurningRight = false;
  isWalking = false; isReverseWalking = false;
  joints.fl_femur = STAND_FL_FEMUR; joints.fl_fibula = STAND_FL_FIBULA;
  joints.fr_femur = STAND_FR_FEMUR;  joints.fr_fibula = STAND_FR_FIBULA;
  joints.bl_femur = STAND_BL_FEMUR;  joints.bl_fibula = STAND_BL_FIBULA;
  joints.br_femur = STAND_BR_FEMUR;  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAngles();
}


bool isInterrupted() {
  if (Serial.available()) {
      char key = Serial.read();
      if (key >= 32) {  // Only process printable characters
        switch (key) {
          case 'w': 
          case 'W':
            if (!isWalking) return true;
          case 's':
          case 'S':
            if (!isReverseWalking) return true;
          case 'a':
          case 'A':
            if (!isTurningLeft) return true;
          case 'd':
          case 'D':
            if (!isTurningRight) return true;
          case 'q':
            if (isTurningLeft || isTurningRight || isWalking || isReverseWalking) {
              return true;
            }          
        }
      }
    }
  return false;
}
bool angleReached(float current, float target, float tolerance = 2.0) {
    return abs(current - target) <= tolerance;
}
bool allJointsReached() {
  return
    angleReached(robot.frontLeft.femur.currentAngle, joints.fl_femur) &&
    angleReached(robot.frontLeft.fibula.currentAngle, joints.fl_fibula) &&

    angleReached(robot.frontRight.femur.currentAngle, joints.fr_femur) &&
    angleReached(robot.frontRight.fibula.currentAngle, joints.fr_fibula) &&

    angleReached(robot.backLeft.femur.currentAngle, joints.bl_femur) &&
    angleReached(robot.backLeft.fibula.currentAngle, joints.bl_fibula) &&

    angleReached(robot.backRight.femur.currentAngle, joints.br_femur) &&
    angleReached(robot.backRight.fibula.currentAngle, joints.br_fibula);
}

void applyJointAnglesSmoothly() {
  applyJointAngles();

  while (!allJointsReached()) {
    robot.update();
    // delay(1);

    if (isInterrupted())
      return;
  }

  // allow physical servo to catch up
  // delay(10); 
}

void jump () {
  // step 1: squat
  joints.fl_fibula = 90;
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
  joints.fl_fibula = 180;
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



void walk_ () {
  
  // step 1: lift front left and back right legs by bending fibulas
  joints.fl_fibula = LIFT_FL_FIBULA;
  joints.br_fibula = LIFT_BR_FIBULA;
  // Serial.println("Lifting front left and back right legs...");
  applyJointAnglesSmoothly();
  // delay(2000);

  // step 2: ready to push by moving femurs

  joints.fl_femur = 180; 
  joints.br_femur = 90; 
  // Serial.println("Moving femurs to push forward...");
  applyJointAnglesSmoothly();
  // delay(2000);

  
  // step 3: lift other
  joints.bl_fibula = 45;
  joints.fl_fibula = STAND_FL_FIBULA;
  joints.bl_femur = 0;
  // Serial.println("Lifting back left and pushing front left...");
  applyJointAnglesSmoothly();
  // delay(2000);

  joints.br_fibula = 180;
  // Serial.println("Moving back right leg...");
  applyJointAnglesSmoothly();
  // delay(2000);


  
  // Serial.println("STANDUP");
  standUp();
  applyJointAnglesSmoothly();

  // delay(2000);



  joints.fr_fibula = LIFT_FR_FIBULA;
  joints.bl_fibula = LIFT_BL_FIBULA;
  // Serial.println("Lifting front right and back left legs...");
  applyJointAnglesSmoothly();
  // delay(2000);


  joints.fr_femur = 0; 
  joints.bl_femur = 90; 
  // Serial.println("Moving femurs to push forward...");
  applyJointAnglesSmoothly();
  // delay(2000);


  joints.br_femur = 160;
  joints.br_fibula = LIFT_BR_FIBULA;
  joints.fr_fibula = 0;
  // Serial.println("Pushing with front right and back right legs...");
  applyJointAnglesSmoothly();
  // delay(2000);


  joints.fr_fibula = 0;
  joints.bl_fibula = 0;
  // Serial.println("Bringing lifted legs back to the ground...");
  applyJointAnglesSmoothly();
  // delay(2000);

  // Serial.println("STANDUP");
  standUp();
  applyJointAnglesSmoothly();

  

}

// const int FL_X = 90; const int FL_Y = 180;
// const int FR_X = 90; const int FR_Y = 0;
// const int BL_X = 90; const int BL_Y = 0;
// const int BR_X = 90; const int BR_Y = 180;
void reverse () {
  // step 1: lift back left and front right legs by bending fibulas
  joints.fr_fibula = LIFT_FR_FIBULA;
  applyJointAnglesSmoothly();
  joints.bl_fibula = LIFT_BL_FIBULA; 
  
  // step 2: move femurs
  joints.bl_femur = BL_Y; joints.fr_femur = FR_X;
  applyJointAnglesSmoothly();
  joints.fl_femur = FL_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.bl_fibula = STAND_BL_FIBULA; 
  applyJointAnglesSmoothly();
  joints.fr_fibula = STAND_FR_FIBULA;
  applyJointAnglesSmoothly();

  standUp();
  applyJointAnglesSmoothly();


  joints.fl_fibula = LIFT_FL_FIBULA;
  applyJointAnglesSmoothly();
  joints.br_fibula = LIFT_BR_FIBULA; 
  
  // step 2: move femurs
  joints.br_femur = BR_Y; joints.fl_femur = FL_X;
  applyJointAnglesSmoothly();
  joints.fr_femur = FR_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAnglesSmoothly();
  joints.fl_fibula = STAND_FL_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();
}

void walk () {
  // step 1: lift back left and front right legs by bending fibulas
  joints.br_fibula = LIFT_BR_FIBULA;
  applyJointAnglesSmoothly();
  joints.fl_fibula = LIFT_FL_FIBULA; 
  
  // step 2: move femurs
  joints.fl_femur = FL_Y; joints.br_femur = BR_X;
  applyJointAnglesSmoothly();
  joints.bl_femur = BL_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fl_fibula = STAND_FL_FIBULA; 
  applyJointAnglesSmoothly();
  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();


  joints.bl_fibula = LIFT_BL_FIBULA;
  applyJointAnglesSmoothly();
  joints.fr_fibula = LIFT_FR_FIBULA; 
  
  // step 2: move femurs
  joints.fr_femur = FR_Y; joints.bl_femur = BL_X;
  applyJointAnglesSmoothly();
  joints.br_femur = BR_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fr_fibula = STAND_FR_FIBULA; 
  applyJointAnglesSmoothly();
  joints.bl_fibula = STAND_BL_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();
}

void walks() {
  isWalking = true;
  // ================= FIRST DIAGONAL =================

  // Lift
  joints.br_fibula = LIFT_BR_FIBULA;
  joints.fl_fibula = LIFT_FL_FIBULA;
  applyJointAnglesSmoothly();

  // Swing
  joints.fl_femur = FL_Y;
  joints.br_femur = BR_X;
  applyJointAnglesSmoothly();

  // Plant
  joints.br_fibula = STAND_BR_FIBULA;
  joints.fl_fibula = STAND_FL_FIBULA;
  applyJointAnglesSmoothly();


  // ================= SECOND DIAGONAL =================

  // Return first pair while lifting second pair
  joints.fl_femur = STAND_FL_FEMUR;
  joints.br_femur = STAND_BR_FEMUR;

  joints.bl_fibula = LIFT_BL_FIBULA;
  joints.fr_fibula = LIFT_FR_FIBULA;
  applyJointAnglesSmoothly();

  // Swing second pair
  joints.bl_femur = BL_X;
  joints.fr_femur = FR_Y;
  applyJointAnglesSmoothly();

  // Plant second pair
  joints.bl_fibula = STAND_BL_FIBULA;
  joints.fr_fibula = STAND_FR_FIBULA;
  applyJointAnglesSmoothly();

  // Prepare for next cycle
  joints.bl_femur = STAND_BL_FEMUR;
  joints.fr_femur = STAND_FR_FEMUR;
  applyJointAnglesSmoothly();

}


void turnLeft () {
  isTurningLeft = true;
  joints.bl_fibula = LIFT_BL_FIBULA;
  applyJointAnglesSmoothly();
  joints.fr_fibula = LIFT_FR_FIBULA; 
  
  // step 2: move femurs
  joints.fr_femur = STAND_FR_FEMUR; joints.bl_femur = BL_X;
  applyJointAnglesSmoothly();
  joints.br_femur = STAND_BR_FEMUR; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fr_fibula = STAND_FR_FIBULA; 
  applyJointAnglesSmoothly();
  joints.bl_fibula = STAND_BL_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();
  // step 1: lift back left and front right legs by bending fibulas
  joints.br_fibula = LIFT_BR_FIBULA;
  applyJointAnglesSmoothly();
  joints.fl_fibula = LIFT_FL_FIBULA; 
  
  // step 2: move femurs
  joints.fl_femur = FL_Y; joints.br_femur = BR_X;
  applyJointAnglesSmoothly();
  joints.bl_femur = BL_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fl_fibula = STAND_FL_FIBULA; 
  applyJointAnglesSmoothly();
  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();


  
}

void turnRight () {
  isTurningRight = true;
  // step 1: lift back left and front right legs by bending fibulas
  joints.br_fibula = LIFT_BR_FIBULA;
  applyJointAnglesSmoothly();
  joints.fl_fibula = LIFT_FL_FIBULA; 
  
  // step 2: move femurs
  joints.fl_femur = STAND_FL_FEMUR; joints.br_femur = BR_X;
  applyJointAnglesSmoothly();
  joints.bl_femur = STAND_BL_FEMUR; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fl_fibula = STAND_FL_FIBULA; 
  applyJointAnglesSmoothly();
  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();


  joints.bl_fibula = LIFT_BL_FIBULA;
  applyJointAnglesSmoothly();
  joints.fr_fibula = LIFT_FR_FIBULA; 
  
  // step 2: move femurs
  joints.fr_femur = FR_Y; joints.bl_femur = STAND_BL_FEMUR;
  applyJointAnglesSmoothly();
  joints.br_femur = BR_Y; 
  applyJointAnglesSmoothly();
  // step 3: bring lifted legs back to the ground
  joints.fr_fibula = STAND_FR_FIBULA; 
  applyJointAnglesSmoothly();
  joints.bl_fibula = STAND_BL_FIBULA;
  applyJointAnglesSmoothly();
  standUp();
  applyJointAnglesSmoothly();
}

void reverseWalk() {

  // ================= FIRST DIAGONAL =================
  isReverseWalking = true;
  // Lift
  joints.fr_fibula = LIFT_FR_FIBULA;
  joints.bl_fibula = LIFT_BL_FIBULA;
  applyJointAnglesSmoothly();

  // Swing backward
  joints.fr_femur = FR_X;
  joints.bl_femur = BL_Y;
  applyJointAnglesSmoothly();

  // Plant
  joints.fr_fibula = STAND_FR_FIBULA;
  joints.bl_fibula = STAND_BL_FIBULA;
  applyJointAnglesSmoothly();


  // ================= SECOND DIAGONAL =================

  // Return first pair while lifting second pair
  joints.fr_femur = STAND_FR_FEMUR;
  joints.bl_femur = STAND_BL_FEMUR;

  joints.fl_fibula = LIFT_FL_FIBULA;
  joints.br_fibula = LIFT_BR_FIBULA;
  applyJointAnglesSmoothly();

  // Swing backward
  joints.fl_femur = FL_X;
  joints.br_femur = BR_Y;
  applyJointAnglesSmoothly();

  // Plant
  joints.fl_fibula = STAND_FL_FIBULA;
  joints.br_fibula = STAND_BR_FIBULA;
  applyJointAnglesSmoothly();

  // Prepare next cycle
  joints.fl_femur = STAND_FL_FEMUR;
  joints.br_femur = STAND_BR_FEMUR;
  applyJointAnglesSmoothly();
}


void handleSerialInput(char key) {
  bool updated = false;
  
  switch(key) {
    // Front Left Femur
    case 's':
    case 'S':
      reverseWalk();
      Serial.println("Reversing...");
      break;
    case 'a':
    case 'A':
      turnLeft();
      Serial.println("turning left...");
      break;
    case 'd':
    case 'D':
      turnRight();
      Serial.println("turning right...");
      break;
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
      walks();
      // reverse();
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

  // ================= SERIAL CONTROL =================
  if (Serial.available()) {

    char key = Serial.read();

    if (key >= 32) {   // printable characters only
      handleSerialInput(key);
    }
  }

  // ================= ESP-NOW CONTROL =================
  char msg[250];
  uint8_t sender[6];

  if (get_latest_message(msg, sizeof(msg), sender)) {

    Serial.print("ESP-NOW Received: ");
    Serial.println(msg[0]);

    // Execute received command
    handleSerialInput(msg[0]);
  }

  // ================= ROBOT UPDATE =================
  robot.update();
}