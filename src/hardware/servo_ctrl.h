#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <stdint.h>

void initializeServos();
void turnHeadLeft();
void turnHeadRight();
void holdHorizontalPosition();
void holdHeadPosition();
void raiseHead();
void returnHeadToCenter();
void updateServoPositions(uint32_t now);

#endif
