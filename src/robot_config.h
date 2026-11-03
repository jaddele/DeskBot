#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <stdint.h>

namespace RobotConfig {

// 最终版硬件接线。所有模块都从这里取引脚，避免出现第二套接线表。
constexpr uint8_t YawServoPin = 13;
constexpr uint8_t PitchServoPin = 12;
constexpr uint8_t TouchPin = 4;
constexpr uint8_t LeftInfraredPin = 5;
constexpr uint8_t RightInfraredPin = 18;
constexpr uint8_t OledSdaPin = 21;
constexpr uint8_t OledSclPin = 22;

// TTP223 默认按下输出 HIGH；如果实测模块为低有效，只改这一项。
constexpr bool TouchActiveHigh = true;
// TTP223 已经有硬件整形，这里的 100 ms 用于过滤安装和线材引入的抖动。
constexpr uint32_t TouchDebounceMilliseconds = 100;
constexpr uint32_t TouchLongPressMilliseconds = 800;

// 红外每 2 ms 采样一次，5 次中至少 4 次 LOW 才认为该侧有障碍物。
constexpr uint8_t InfraredSampleCount = 5;
constexpr uint8_t InfraredRequiredActiveSamples = 4;
constexpr uint32_t InfraredSampleIntervalMilliseconds = 2;

// 舵机角度限制和步进速度。当前软件没有角度反馈，角度是最后写出的 PWM 角度。
constexpr int ServoPulseMinimumMicroseconds = 1000;
constexpr int ServoPulseMaximumMicroseconds = 2000;
constexpr int HorizontalMinimumAngle = 0;
constexpr int HorizontalMaximumAngle = 180;
constexpr int VerticalMinimumAngle = 40;
constexpr int VerticalMaximumAngle = 140;
constexpr int CenterAngle = 90;
constexpr int RaisedHeadAngle = 120;
constexpr uint32_t ServoStepIntervalMilliseconds = 20;

// 触摸或红外目标消失后的行为计时，以及 OLED 动画刷新节拍。
constexpr uint32_t ReturnToCenterDelayMilliseconds = 3000;
constexpr uint32_t FaceRefreshIntervalMilliseconds = 50;

} // namespace RobotConfig

#endif
