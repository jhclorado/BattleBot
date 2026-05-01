#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#define SERVOMIN 170
#define SERVOMAX 650

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

const int servoCount = 10; // Number of servos you have
const int servoPins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; // Define the PCA9685 pins for your servos

int defaultPositions[servoCount] = {0, 0, 0, 40, 0, 0, 0, 80, 45, 90}; // Array to store default positions for each servo
int servoAngles[servoCount]; // Array to store current servo angles

void setup()
{
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(60);

  // Initialize servo angles to default positions
  for (int i = 0; i < servoCount; i++)
  {
    servoAngles[i] = defaultPositions[i];
    pwm.setPWM(servoPins[i], 0, angleToPulse(servoAngles[i]));
  }
}

void loop()
{
  // Check for incoming serial data
  if (Serial.available() > 0)
  {
    // Read the incoming byte
    char command = Serial.read();

    // Handle the command
    switch (command)
    {
    case 'q':
      moveServo(0, 0);
      break;
    case '1':
      moveServo(0, 180);
      break;
    //index

    case 'w':
        moveServo(1, 0);
        break;
      case '2':
        moveServo(1, 180);
        break; 
      //ring

      case 'e':
        moveServo(2, 0);
        break;
      case '3':
        moveServo(2, 180);
        break; 
      //pinky
//.......................................thumb section start.................................................
      case 'r':
        moveServo(3, 40);
        break;
      case '4':
        moveServo(3, 140);
        break; 
      //thumb up & down

      case 't':
        moveServo(4, 0);
        break;
      case '5':
        moveServo(4, 120);
        break; 
      //thumb forward & back

      case 'y':
        moveServo(5, 0);
        break;
      case '6':
        moveServo(5, 180);
        break;  
      //thumb close & open
//..........................................thumb section end.................................................
      case 'u':
        moveServo(6, 0);
        break;
      case '7':
        moveServo(6, 180);
        break;  
      //middle
//...........................................wrist section start..............................................
      case 'i':
        moveServo(7, 55);
        break;
      case '8':
        moveServo(7, 110);
        break;  
      //wrist left & right

      case 'o':
        moveServo(8, 0);
        break;
      case '9':
        moveServo(8, 130);
        break;  
      //wrist forward & back
//...........................................wrist section end..................................................
      case 'p':
        moveServo(9, 70);
        break;
      case '10':
        moveServo(9, 140);
        break;
      //spread
    
    case 'a':
      setDefaultPositions();
      break;
    }
  }
}

void moveServo(int servoIndex, int targetAngle)
{
  // Smoothly move the servo to the target angle
  for (int i = servoAngles[servoIndex]; i != targetAngle; i += (targetAngle > i) ? 1 : -1)
  {
    servoAngles[servoIndex] = i;
    pwm.setPWM(servoPins[servoIndex], 0, angleToPulse(i));
    delay(10); // Adjust the delay for smoother motion
  }
}

void setDefaultPositions()
{
  // Set all servos to their default positions
  for (int i = 0; i < servoCount; i++)
  {
    moveServo(i, defaultPositions[i]);
  }
}

int angleToPulse(int angle)
{
  // Convert angle to servo pulse length
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}