#ifndef INPUT_FILTERS_H
#define INPUT_FILTERS_H

#include <stdint.h>

#include "hardware/ir_sensor.h"

struct TouchEvents {
    bool pressed = false;
    bool pressedEvent = false;
    bool releasedEvent = false;
    bool longPressEvent = false;
};

class TouchFilter {
public:
    // 初始化原始电平记忆。启动时即便手已经放在模块上，也要经过消抖。
    void reset(uint32_t now, bool rawPressed);

    // 输入当前原始电平，返回本轮的稳定状态和一次性事件。
    TouchEvents update(uint32_t now, bool rawPressed);

private:
    bool lastRawPressed_ = false;
    bool stablePressed_ = false;
    bool longPressReported_ = false;
    uint32_t rawChangedAt_ = 0;
    uint32_t pressedAt_ = 0;
};

class InfraredFilter {
public:
    // 清空当前 5 次多数表决窗口。
    void reset();

    // 加入一组左右同时采样；完成窗口时更新 direction 并返回 true。
    bool addSample(bool leftActive, bool rightActive, HandDirection &direction);

private:
    uint8_t sampleCount_ = 0;
    uint8_t leftActiveCount_ = 0;
    uint8_t rightActiveCount_ = 0;
};

class InfraredSampler {
public:
    // 清空方向缓存并重新开始 2 ms 采样节拍。
    void reset(uint32_t now);

    // 按固定节拍采样；未完成 5 次表决时只返回缓存方向。
    InfraredReading update(uint32_t now, bool leftActive, bool rightActive);

private:
    InfraredFilter filter_;
    HandDirection currentDirection_ = HandDirection::None;
    uint32_t lastSampleAt_ = 0;
};

#endif
