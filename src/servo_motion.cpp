#include "servo_motion.h"

#include "robot_config.h"

namespace {

int moveOneDegree(int current, int target)
{
    // 只走一步，避免舵机瞬间跳转造成机械冲击和电流尖峰。
    if (current < target) {
        return current + 1;
    }
    if (current > target) {
        return current - 1;
    }
    return current;
}

} // namespace

void ServoMotion::reset(uint32_t now)
{
    currentHorizontalAngle_ = RobotConfig::CenterAngle;
    currentVerticalAngle_ = RobotConfig::CenterAngle;
    targetHorizontalAngle_ = RobotConfig::CenterAngle;
    targetVerticalAngle_ = RobotConfig::CenterAngle;
    lastStepAt_ = now;
}

void ServoMotion::turnLeft()
{
    // 目标基于 current 而非旧 target，能在归中途中立即反向。
    targetHorizontalAngle_ = currentHorizontalAngle_ > RobotConfig::HorizontalMinimumAngle
        ? currentHorizontalAngle_ - 1
        : RobotConfig::HorizontalMinimumAngle;
}

void ServoMotion::turnRight()
{
    targetHorizontalAngle_ = currentHorizontalAngle_ < RobotConfig::HorizontalMaximumAngle
        ? currentHorizontalAngle_ + 1
        : RobotConfig::HorizontalMaximumAngle;
}

void ServoMotion::holdHorizontal()
{
    // 将 yaw 目标拉回当前输出角，清除之前的转头命令。
    targetHorizontalAngle_ = currentHorizontalAngle_;
}

void ServoMotion::holdHead()
{
    // 两轴都锁在当前输出角，用于红外丢失和双侧检测。
    targetHorizontalAngle_ = currentHorizontalAngle_;
    targetVerticalAngle_ = currentVerticalAngle_;
}

void ServoMotion::raiseHead()
{
    // 120° 已按实机方向约定为抬头角度。
    targetVerticalAngle_ = RobotConfig::RaisedHeadAngle;
}

void ServoMotion::returnToCenter()
{
    // 只设置目标，舵机仍由 update() 平滑回到 90°。
    targetHorizontalAngle_ = RobotConfig::CenterAngle;
    targetVerticalAngle_ = RobotConfig::CenterAngle;
}

bool ServoMotion::update(uint32_t now)
{
    if (now - lastStepAt_ < RobotConfig::ServoStepIntervalMilliseconds) {
        return false;
    }
    lastStepAt_ = now;

    const int previousHorizontal = currentHorizontalAngle_;
    const int previousVertical = currentVerticalAngle_;
    currentHorizontalAngle_ = moveOneDegree(currentHorizontalAngle_, targetHorizontalAngle_);
    currentVerticalAngle_ = moveOneDegree(currentVerticalAngle_, targetVerticalAngle_);
    return currentHorizontalAngle_ != previousHorizontal ||
           currentVerticalAngle_ != previousVertical;
}

int ServoMotion::currentHorizontalAngle() const
{
    return currentHorizontalAngle_;
}

int ServoMotion::currentVerticalAngle() const
{
    return currentVerticalAngle_;
}
