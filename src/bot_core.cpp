#include "bot_core.h"

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_SSD1306.h>

#include "bot_behavior.h"
#include "hardware/ir_sensor.h"
#include "hardware/oled.h"
#include "hardware/servo_ctrl.h"
#include "hardware/touch.h"
#include "robot_config.h"

namespace {

constexpr uint8_t ScreenWidth = 128;
constexpr uint8_t ScreenHeight = 64;

// 这些对象只在本文件中存在，避免 main.cpp 直接依赖具体硬件对象。
Adafruit_SSD1306 display(ScreenWidth, ScreenHeight, &Wire, -1);
BotBehavior behavior;
BotFace lastFace = BotFace::Default;
uint32_t lastFaceRefreshAt = 0;

// 将核心逻辑使用的轻量表情类型转换为 OLED 模块的公共枚举。
// OLED 模块仍只负责绘制，不参与传感器优先级判断。
FaceEmotion toFaceEmotion(BotFace face)
{
    switch (face) {
        case BotFace::Happy:
            return FaceEmotion::Happy;
        case BotFace::Confused:
            return FaceEmotion::Confused;
        case BotFace::Heart:
            return FaceEmotion::Heart;
        case BotFace::Default:
            return FaceEmotion::Default;
    }
    return FaceEmotion::Default;
}

void applyMovement(const BehaviorOutput &output)
{
    // Center 是完整归中命令，必须同时清除 yaw 和 pitch 的旧目标。
    if (output.horizontal == HorizontalAction::Center &&
        output.vertical == VerticalAction::Center) {
        returnHeadToCenter();
        return;
    }

    // 两轴都保持时使用一次整体停止，避免某一轴继续追赶旧目标。
    if (output.horizontal == HorizontalAction::Hold &&
        output.vertical == VerticalAction::Hold) {
        holdHeadPosition();
        return;
    }

    switch (output.horizontal) {
        case HorizontalAction::TurnLeft:
            turnHeadLeft();
            break;
        case HorizontalAction::TurnRight:
            turnHeadRight();
            break;
        case HorizontalAction::Hold:
            holdHorizontalPosition();
            break;
        case HorizontalAction::Center:
            break;
    }

    if (output.vertical == VerticalAction::Raise) {
        raiseHead();
    }
}

} // namespace

void initializeBot()
{
    // 初始化顺序：先建立输入和舵机，再初始化 OLED；所有模块的引脚
    // 都从 robot_config.h 读取，避免在多个文件中出现不同接线版本。
    initializeInfraredSensors();
    initializeTouchSensor();
    initializeServos();
    init_Oled(display);

    const uint32_t now = millis();
    behavior.reset(now);
    lastFace = BotFace::Default;
    setFaceEmotion(FaceEmotion::Default);
    updateFace(display);
    lastFaceRefreshAt = now;
}

void updateBot()
{
    const uint32_t now = millis();

    // 触摸先于红外读取，行为模块随后会把触摸作为最高优先级输入。
    const TouchEvents touch = updateTouchSensor(now);
    if (touch.releasedEvent) {
        // 丢弃触摸前尚未完成的红外采样窗口；后续必须先完成一个
        // 全部无遮挡的窗口，新的遮挡才能接管动作。
        resetInfraredSampling(now);
    }
    const InfraredReading infrared = updateInfraredSensors(now);

    // 组装一份本轮快照交给纯逻辑模块。硬件模块不直接改变表情或舵机。
    BehaviorInput input;
    input.now = now;
    input.infrared = infrared.direction;
    input.infraredSampleComplete = infrared.sampleWindowComplete;
    input.touchPressed = touch.pressed;
    input.touchPressedEvent = touch.pressedEvent;
    input.touchReleasedEvent = touch.releasedEvent;
    input.touchLongPressEvent = touch.longPressEvent;

    // 行为模块只返回目标动作；硬件适配和 OLED 动画分别在这里执行。
    const BehaviorOutput output = behavior.update(input);
    applyMovement(output);
    updateServoPositions(now);

    if (output.face != lastFace) {
        lastFace = output.face;
        setFaceEmotion(toFaceEmotion(output.face));
    }

    if (now - lastFaceRefreshAt >= RobotConfig::FaceRefreshIntervalMilliseconds) {
        updateFace(display);
        lastFaceRefreshAt = now;
    }
}
