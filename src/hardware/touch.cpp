#include "touch.h"

#include <Arduino.h>

#include "robot_config.h"

namespace {

TouchFilter touchFilter;

bool readTouchActive()
{
    // 先读物理 HIGH，再由配置转换成逻辑上的“正在触摸”。
    const bool pinIsHigh = digitalRead(RobotConfig::TouchPin) == HIGH;
    return RobotConfig::TouchActiveHigh ? pinIsHigh : !pinIsHigh;
}

} // namespace

void initializeTouchSensor()
{
    // TTP223 使用独立数字输出，不启用 ESP32 内部上拉或下拉。
    pinMode(RobotConfig::TouchPin, INPUT);
    touchFilter.reset(millis(), readTouchActive());
}

TouchEvents updateTouchSensor(uint32_t now)
{
    // 这一层只做 GPIO 读取；消抖和长按语义由可测试的 TouchFilter 处理。
    return touchFilter.update(now, readTouchActive());
}
