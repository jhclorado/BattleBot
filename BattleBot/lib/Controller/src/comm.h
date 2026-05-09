#ifndef COMM_H
#define COMM_H

#include <Arduino.h>
#include "legs.h"

class Controller {
private:
  Quadruped* robot;

public:
  Controller(Quadruped* r) {
    robot = r;
  }

  void parseAndApply(const String& msg) {
    int fl1, fl2, fr1, fr2, bl1, bl2, br1, br2;

    int ok = sscanf(
      msg.c_str(),
      "FL:%d,%d;FR:%d,%d;BL:%d,%d;BR:%d,%d",
      &fl1, &fl2,
      &fr1, &fr2,
      &bl1, &bl2,
      &br1, &br2
    );

    if (ok == 8) {
      robot->frontLeft.setTarget(fl1, fl2);
      robot->frontRight.setTarget(fr1, fr2);
      robot->backLeft.setTarget(bl1, bl2);
      robot->backRight.setTarget(br1, br2);
    } else {
      Serial.println("[Controller] Bad format");
    }
  }

  void handlePacket(const uint8_t* data, size_t len) {
    String msg;
    for (int i = 0; i < len; i++) msg += (char)data[i];

    Serial.println(msg);
  }
};

#endif
