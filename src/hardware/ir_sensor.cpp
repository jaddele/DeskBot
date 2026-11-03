#include "ir_sensor.h"

#include <Arduino.h>

#include "input_filters.h"
#include "robot_config.h"

namespace {

InfraredSampler infraredSampler;

} // namespace

void initializeInfraredSensors()
{
    // 红外 OUT 是推挽数字信号，LOW 表示模块检测到障碍物。
    pinMode(RobotConfig::LeftInfraredPin, INPUT);
    pinMode(RobotConfig::RightInfraredPin, INPUT);
    infraredSampler.reset(millis());
}

void resetInfraredSampling(uint32_t now)
{
    infraredSampler.reset(now);
}

InfraredReading updateInfraredSensors(uint32_t now)
{
    // 两路在同一轮读取；多数表决和 2 ms 节拍由 InfraredSampler 处理。
    const bool leftActive = digitalRead(RobotConfig::LeftInfraredPin) == LOW;
    const bool rightActive = digitalRead(RobotConfig::RightInfraredPin) == LOW;
    return infraredSampler.update(now, leftActive, rightActive);
}
