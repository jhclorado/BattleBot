#ifndef LEGS_H
#define LEGS_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVOMIN 170
#define SERVOMAX 650

// External PWM driver reference (defined in main.cpp)
extern Adafruit_PWMServoDriver pwm;

// Convert angle (0-180) to pulse width
int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

class Joint {
public:
  int servoIndex;
  int currentAngle = 0;
  int targetAngle = 0;
  unsigned long lastUpdate = 0;
  int stepDelay = 50;

  Joint(int index) {
    servoIndex = index;
  }

  void setTarget(int angle) {
    targetAngle = angle;
  }

  void update() {
    
    /* 
    
    FOR SMOOTH MOVEMENT
    unsigned long now = millis();
    if (now - lastUpdate >= stepDelay) {
      lastUpdate = now;

      if (currentAngle < targetAngle) {
        currentAngle++;
        pwm.setPWM(servoIndex, 0, angleToPulse(currentAngle));
      } 
      else if (currentAngle > targetAngle) {
        currentAngle--;
        pwm.setPWM(servoIndex, 0, angleToPulse(currentAngle));
      }
    }
    */

    if (currentAngle < targetAngle) {
        currentAngle++;
        pwm.setPWM(servoIndex, 0, angleToPulse(currentAngle));
    } 
    
    else if (currentAngle > targetAngle) {
      currentAngle--;
      pwm.setPWM(servoIndex, 0, angleToPulse(currentAngle));
    }
    
  }
};


class Femur : public Joint {
public:
  Femur(int pin) : Joint(pin) {}
};

class Fibula : public Joint {
public:
  Fibula(int pin) : Joint(pin) {}
};


class Limb {
public:
  Femur femur;
  Fibula fibula;

  Limb(int femurPin, int fibulaPin)
    : femur(femurPin), fibula(fibulaPin) {}

  void setTarget(int femurAngle, int fibulaAngle) {
    femur.setTarget(femurAngle);
    fibula.setTarget(fibulaAngle);
  }

  void update() {
    femur.update();
    fibula.update();
  }
};

class Quadruped {
public:
  Limb frontLeft;
  Limb frontRight;
  Limb backLeft;
  Limb backRight;

  Quadruped(
    int fl_femur, int fl_fibula,
    int fr_femur, int fr_fibula,
    int bl_femur, int bl_fibula,
    int br_femur, int br_fibula
  )
    : frontLeft(fl_femur, fl_fibula),
      frontRight(fr_femur, fr_fibula),
      backLeft(bl_femur, bl_fibula),
      backRight(br_femur, br_fibula) {}

  void update() {
    frontLeft.update();
    frontRight.update();
    backLeft.update();
    backRight.update();
  }
};

#endif