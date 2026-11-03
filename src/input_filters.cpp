#include "input_filters.h"

#include "robot_config.h"

void TouchFilter::reset(uint32_t now, bool rawPressed)
{
    // stablePressed_ 故意从 false 开始，避免上电时直接把噪声当成事件。
    lastRawPressed_ = rawPressed;
    stablePressed_ = false;
    longPressReported_ = false;
    rawChangedAt_ = now;
    pressedAt_ = now;
}

TouchEvents TouchFilter::update(uint32_t now, bool rawPressed)
{
    TouchEvents events;

    if (rawPressed != lastRawPressed_) {
        // 用原始下降沿锁定实际接触时长，避免恰好在 800 ms 松手时
        // 因为进入松手消抖而漏掉长按事件。
        if (lastRawPressed_ && !rawPressed && stablePressed_ &&
            !longPressReported_ &&
            now - pressedAt_ >= RobotConfig::TouchLongPressMilliseconds) {
            longPressReported_ = true;
            events.longPressEvent = true;
        }
        lastRawPressed_ = rawPressed;
        rawChangedAt_ = now;
    }

    if (rawPressed != stablePressed_ &&
        now - rawChangedAt_ >= RobotConfig::TouchDebounceMilliseconds) {
        // 原始电平连续稳定 100 ms 后，才提交按下或松开状态变化。
        stablePressed_ = rawPressed;
        if (stablePressed_) {
            pressedAt_ = now;
            longPressReported_ = false;
            events.pressedEvent = true;
        } else {
            events.releasedEvent = true;
            longPressReported_ = false;
        }
    }

    if (stablePressed_ && rawPressed && !longPressReported_ &&
        now - pressedAt_ >= RobotConfig::TouchLongPressMilliseconds) {
        // 长按只发一次；后续循环只保持 Heart 表情，不重复触发动画。
        longPressReported_ = true;
        events.longPressEvent = true;
    }

    events.pressed = stablePressed_;
    return events;
}

void InfraredFilter::reset()
{
    sampleCount_ = 0;
    leftActiveCount_ = 0;
    rightActiveCount_ = 0;
}

bool InfraredFilter::addSample(bool leftActive, bool rightActive,
                               HandDirection &direction)
{
    if (leftActive) {
        leftActiveCount_++;
    }
    if (rightActive) {
        rightActiveCount_++;
    }
    sampleCount_++;

    if (sampleCount_ < RobotConfig::InfraredSampleCount) {
        // 窗口未满时不改变上一轮稳定方向。
        return false;
    }

    const bool leftDetected =
        leftActiveCount_ >= RobotConfig::InfraredRequiredActiveSamples;
    const bool rightDetected =
        rightActiveCount_ >= RobotConfig::InfraredRequiredActiveSamples;

    if (leftDetected && rightDetected) {
        direction = HandDirection::Both;
    } else if (leftDetected) {
        direction = HandDirection::Left;
    } else if (rightDetected) {
        direction = HandDirection::Right;
    } else {
        direction = HandDirection::None;
    }

    reset();
    return true;
}

void InfraredSampler::reset(uint32_t now)
{
    filter_.reset();
    currentDirection_ = HandDirection::None;
    lastSampleAt_ = now;
}

InfraredReading InfraredSampler::update(uint32_t now,
                                        bool leftActive,
                                        bool rightActive)
{
    if (now - lastSampleAt_ < RobotConfig::InfraredSampleIntervalMilliseconds) {
        // 这里不阻塞等待；调用方下一轮 loop 再继续采样。
        return InfraredReading(currentDirection_, false);
    }
    lastSampleAt_ = now;

    const bool complete = filter_.addSample(leftActive, rightActive,
                                            currentDirection_);
    return InfraredReading(currentDirection_, complete);
}
