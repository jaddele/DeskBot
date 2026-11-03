#ifndef SERVO_MOTION_H
#define SERVO_MOTION_H

#include <stdint.h>

class ServoMotion {
public:
    // 让软件角度与上电后舵机的中位假设一致。
    void reset(uint32_t now);

    // 每次调用只更新目标角度，真正写 PWM 在 update() 中按 20 ms 节拍执行。
    void turnLeft();
    void turnRight();
    void holdHorizontal();
    void holdHead();
    void raiseHead();
    void returnToCenter();
    // 按当前目标每 20 ms 最多移动 1°，返回本轮是否发生角度变化。
    bool update(uint32_t now);

    int currentHorizontalAngle() const;
    int currentVerticalAngle() const;

private:
    int currentHorizontalAngle_ = 90;
    int currentVerticalAngle_ = 90;
    int targetHorizontalAngle_ = 90;
    int targetVerticalAngle_ = 90;
    uint32_t lastStepAt_ = 0;
};

#endif
