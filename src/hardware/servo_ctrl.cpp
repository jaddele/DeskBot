#include "servo_ctrl.h"

#include <Arduino.h>
#include <ESP32Servo.h>

#include "robot_config.h"
#include "servo_motion.h"

namespace {

Servo yawServo;
Servo pitchServo;
ServoMotion motion;

} // namespace

void initializeServos()
{
    // 舵机使用外接 5V，信号脚只由 ESP32 输出 PWM；供电地必须共地。
    yawServo.attach(RobotConfig::YawServoPin,
                    RobotConfig::ServoPulseMinimumMicroseconds,
                    RobotConfig::ServoPulseMaximumMicroseconds);
    pitchServo.attach(RobotConfig::PitchServoPin,
                      RobotConfig::ServoPulseMinimumMicroseconds,
                      RobotConfig::ServoPulseMaximumMicroseconds);

    motion.reset(millis());
    yawServo.write(motion.currentHorizontalAngle());
    pitchServo.write(motion.currentVerticalAngle());
}

void turnHeadLeft()
{
    motion.turnLeft();
}

void turnHeadRight()
{
    motion.turnRight();
}

void holdHorizontalPosition()
{
    motion.holdHorizontal();
}

void holdHeadPosition()
{
    motion.holdHead();
}

void raiseHead()
{
    motion.raiseHead();
}

void returnHeadToCenter()
{
    motion.returnToCenter();
}

void updateServoPositions(uint32_t now)
{
    // 只有角度实际变化时才写一次 Servo，减少无意义的 PWM 更新。
    if (!motion.update(now)) {
        return;
    }

    yawServo.write(motion.currentHorizontalAngle());
    pitchServo.write(motion.currentVerticalAngle());
}
