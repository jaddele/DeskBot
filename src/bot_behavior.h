#ifndef BOT_BEHAVIOR_H
#define BOT_BEHAVIOR_H

#include <stdint.h>

#include "hardware/ir_sensor.h"

// yaw 轴在当前循环应该执行的动作。
enum class HorizontalAction : uint8_t {
    Hold,
    TurnLeft,
    TurnRight,
    Center
};

// pitch 轴在当前循环应该执行的动作。
enum class VerticalAction : uint8_t {
    Hold,
    Raise,
    Center
};

// 核心逻辑使用的表情集合；具体绘制由 hardware/oled.cpp 完成。
enum class BotFace : uint8_t {
    Default,
    Happy,
    Confused,
    Heart
};

struct BehaviorInput {
    uint32_t now = 0;                         // 当前 millis() 时间戳
    HandDirection infrared = HandDirection::None; // 最近一次稳定红外方向
    bool infraredSampleComplete = true;      // 本轮是否完成 5 次红外表决
    bool touchPressed = false;                // 消抖后的持续按下状态
    bool touchPressedEvent = false;           // 消抖后的按下沿，只出现一轮
    bool touchReleasedEvent = false;          // 消抖后的松开沿，只出现一轮
    bool touchLongPressEvent = false;         // 满 800 ms 的一次性事件
};

struct BehaviorOutput {
    HorizontalAction horizontal = HorizontalAction::Hold;
    VerticalAction vertical = VerticalAction::Hold;
    BotFace face = BotFace::Default;

    BehaviorOutput(HorizontalAction horizontalAction = HorizontalAction::Hold,
                   VerticalAction verticalAction = VerticalAction::Hold,
                   BotFace faceValue = BotFace::Default)
        : horizontal(horizontalAction), vertical(verticalAction), face(faceValue)
    {
    }
};

class BotBehavior {
public:
    // 清空上一次互动，通常在 setup() 中调用。
    void reset(uint32_t now);

    // 根据一轮输入返回动作目标；本函数不直接访问 GPIO、Servo 或 OLED。
    BehaviorOutput update(const BehaviorInput &input);

private:
    enum class TouchMode : uint8_t {
        Inactive,
        Pressed,
        Waiting,
        Returning
    };

    TouchMode touchMode_ = TouchMode::Inactive; // 触摸优先级状态
    bool longTouch_ = false;                    // 本次触摸是否已进入爱心状态
    bool infraredClearSeenAfterTouch_ = false;  // 松手后是否完成无遮挡确认
    uint32_t touchReleasedAt_ = 0;              // 触摸等待归中的起点

    bool infraredTrackingWasActive_ = false;    // 上一轮是否正在跟随目标
    bool infraredLossTimerActive_ = false;      // 红外丢失 3 秒计时是否运行
    uint32_t infraredLostAt_ = 0;               // 红外目标最后消失时间
};

#endif
