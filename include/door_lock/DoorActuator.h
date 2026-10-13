#pragma once

#include <ESP32Servo.h>

#include "Ports.h"

class DoorActuator : public DoorOutputPort {
 public:
  DoorActuator(byte servoPin, byte ledPin, int lockedAngle, int unlockedAngle)
      : servoPin_(servoPin),
        ledPin_(ledPin),
        lockedAngle_(lockedAngle),
        unlockedAngle_(unlockedAngle) {}

  void begin() {
    pinMode(ledPin_, OUTPUT);
    digitalWrite(ledPin_, LOW);
    servo_.setPeriodHertz(50);
    servo_.attach(servoPin_, 500, 2400);
    servo_.write(lockedAngle_);
  }

  void open() override {
    servo_.write(unlockedAngle_);
    digitalWrite(ledPin_, HIGH);
  }

  void close() override {
    servo_.write(lockedAngle_);
    digitalWrite(ledPin_, LOW);
  }

 private:
  Servo servo_;
  byte servoPin_;
  byte ledPin_;
  int lockedAngle_;
  int unlockedAngle_;
};