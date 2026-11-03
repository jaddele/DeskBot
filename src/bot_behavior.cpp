#include "bot_behavior.h"

#include "robot_config.h"

namespace {

BehaviorOutput followInfrared(HandDirection direction)
{
    // 左右单侧目标会让 yaw 继续朝对应方向走；pitch 同时抬到 120°。
    // Both 只保持水平位置，避免两侧同时有人时头部来回争抢方向。
    switch (direction) {
        case HandDirection::Left:
            return {HorizontalAction::TurnLeft, VerticalAction::Raise, BotFace::Happy};
        case HandDirection::Right:
            return {HorizontalAction::TurnRight, VerticalAction::Raise, BotFace::Happy};
        case HandDirection::Both:
            return {HorizontalAction::Hold, VerticalAction::Raise, BotFace::Default};
        case HandDirection::None:
            break;
    }

    return {HorizontalAction::Center, VerticalAction::Center, BotFace::Default};
}

} // namespace

void BotBehavior::reset(uint32_t now)
{
    // reset 不移动硬件，只清除行为记忆；当前角度由舵机模块独立维护。
    touchMode_ = TouchMode::Inactive;
    longTouch_ = false;
    infraredClearSeenAfterTouch_ = false;
    touchReleasedAt_ = now;
    infraredTrackingWasActive_ = false;
    infraredLossTimerActive_ = false;
    infraredLostAt_ = now;
}

BehaviorOutput BotBehavior::update(const BehaviorInput &input)
{
    // 新的稳定触摸可以打断红外跟随、红外归中和上一次触摸等待。
    if (input.touchPressedEvent) {
        touchMode_ = TouchMode::Pressed;
        longTouch_ = false;
        infraredClearSeenAfterTouch_ = false;
        infraredLossTimerActive_ = false;
        infraredTrackingWasActive_ = false;
    }

    if (touchMode_ == TouchMode::Pressed) {
        // 按住期间始终锁住 yaw，并保持抬头；红外不能抢占触摸。
        if (input.touchLongPressEvent) {
            longTouch_ = true;
        }

        if (input.touchReleasedEvent) {
            touchMode_ = TouchMode::Waiting;
            touchReleasedAt_ = input.now;
            infraredClearSeenAfterTouch_ = false;
        }

        return {HorizontalAction::Hold,
                VerticalAction::Raise,
                longTouch_ ? BotFace::Heart : BotFace::Confused};
    }

    if (touchMode_ == TouchMode::Waiting || touchMode_ == TouchMode::Returning) {
        // 只有“完整采样窗口”才可以改变触摸后的红外解锁状态。
        if (input.infraredSampleComplete) {
            if (input.infrared == HandDirection::None) {
                infraredClearSeenAfterTouch_ = true;
            } else if (infraredClearSeenAfterTouch_) {
                touchMode_ = TouchMode::Inactive;
                infraredTrackingWasActive_ = true;
                infraredLossTimerActive_ = false;
                return followInfrared(input.infrared);
            }
        }

        if (touchMode_ == TouchMode::Waiting &&
            input.now - touchReleasedAt_ >= RobotConfig::ReturnToCenterDelayMilliseconds) {
            touchMode_ = TouchMode::Returning;
        }

        if (touchMode_ == TouchMode::Waiting) {
            return {HorizontalAction::Hold,
                    VerticalAction::Raise,
                    longTouch_ ? BotFace::Heart : BotFace::Confused};
        }

        return {HorizontalAction::Center, VerticalAction::Center, BotFace::Default};
    }

    if (input.infrared != HandDirection::None) {
        // 触摸锁定结束后，新的稳定红外方向恢复普通跟随。
        infraredTrackingWasActive_ = true;
        infraredLossTimerActive_ = false;
        return followInfrared(input.infrared);
    }

    if (infraredTrackingWasActive_) {
        // 目标刚消失：先停在原地显示疑惑，计时满 3 秒才归中。
        infraredTrackingWasActive_ = false;
        infraredLossTimerActive_ = true;
        infraredLostAt_ = input.now;
    }

    if (infraredLossTimerActive_) {
        if (input.now - infraredLostAt_ < RobotConfig::ReturnToCenterDelayMilliseconds) {
            return {HorizontalAction::Hold, VerticalAction::Hold, BotFace::Confused};
        }
        infraredLossTimerActive_ = false;
    }

    return {HorizontalAction::Center, VerticalAction::Center, BotFace::Default};
}
