#include <unity.h>

#include "bot_behavior.h"
#include "input_filters.h"
#include "servo_motion.h"

namespace {

BehaviorInput inputAt(uint32_t now, HandDirection infrared)
{
    BehaviorInput input{};
    input.now = now;
    input.infrared = infrared;
    return input;
}

void assertOutput(const BehaviorOutput &output,
                  HorizontalAction horizontal,
                  VerticalAction vertical,
                  BotFace face)
{
    TEST_ASSERT_EQUAL_INT(static_cast<int>(horizontal),
                          static_cast<int>(output.horizontal));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(vertical),
                          static_cast<int>(output.vertical));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(face),
                          static_cast<int>(output.face));
}

void test_touch_preempts_tracking_and_holds_horizontal()
{
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorOutput output = behavior.update(inputAt(10, HandDirection::Right));
    assertOutput(output, HorizontalAction::TurnRight,
                 VerticalAction::Raise, BotFace::Happy);

    BehaviorInput touch = inputAt(100, HandDirection::Right);
    touch.touchPressed = true;
    touch.touchPressedEvent = true;
    output = behavior.update(touch);

    assertOutput(output, HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Confused);
}

void test_long_touch_keeps_heart_until_three_second_timeout()
{
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorInput input = inputAt(100, HandDirection::Right);
    input.touchPressed = true;
    input.touchPressedEvent = true;
    assertOutput(behavior.update(input), HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Confused);

    input = inputAt(900, HandDirection::Right);
    input.touchPressed = true;
    input.touchLongPressEvent = true;
    assertOutput(behavior.update(input), HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Heart);

    input = inputAt(1000, HandDirection::Right);
    input.touchReleasedEvent = true;
    assertOutput(behavior.update(input), HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Heart);

    assertOutput(behavior.update(inputAt(3999, HandDirection::Right)),
                 HorizontalAction::Hold, VerticalAction::Raise, BotFace::Heart);
    assertOutput(behavior.update(inputAt(4000, HandDirection::Right)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
}

void test_short_touch_requires_clear_then_new_infrared_obstruction()
{
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorInput input = inputAt(100, HandDirection::Right);
    input.touchPressed = true;
    input.touchPressedEvent = true;
    behavior.update(input);

    input = inputAt(200, HandDirection::Right);
    input.touchReleasedEvent = true;
    assertOutput(behavior.update(input), HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Confused);
    assertOutput(behavior.update(inputAt(300, HandDirection::Right)),
                 HorizontalAction::Hold, VerticalAction::Raise, BotFace::Confused);
    assertOutput(behavior.update(inputAt(400, HandDirection::None)),
                 HorizontalAction::Hold, VerticalAction::Raise, BotFace::Confused);
    assertOutput(behavior.update(inputAt(500, HandDirection::Left)),
                 HorizontalAction::TurnLeft, VerticalAction::Raise, BotFace::Happy);
}

void test_old_infrared_stays_blocked_after_timeout_until_clear()
{
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorInput input = inputAt(10, HandDirection::Right);
    input.touchPressed = true;
    input.touchPressedEvent = true;
    behavior.update(input);
    input = inputAt(20, HandDirection::Right);
    input.touchReleasedEvent = true;
    behavior.update(input);

    assertOutput(behavior.update(inputAt(3020, HandDirection::Right)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
    assertOutput(behavior.update(inputAt(3300, HandDirection::Right)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
    behavior.update(inputAt(3400, HandDirection::None));
    assertOutput(behavior.update(inputAt(3500, HandDirection::Right)),
                 HorizontalAction::TurnRight, VerticalAction::Raise, BotFace::Happy);
}

void test_lost_infrared_waits_three_seconds_then_centers()
{
    BotBehavior behavior;
    behavior.reset(0);

    assertOutput(behavior.update(inputAt(10, HandDirection::Left)),
                 HorizontalAction::TurnLeft, VerticalAction::Raise, BotFace::Happy);
    assertOutput(behavior.update(inputAt(20, HandDirection::None)),
                 HorizontalAction::Hold, VerticalAction::Hold, BotFace::Confused);
    assertOutput(behavior.update(inputAt(3019, HandDirection::None)),
                 HorizontalAction::Hold, VerticalAction::Hold, BotFace::Confused);
    assertOutput(behavior.update(inputAt(3020, HandDirection::None)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
    assertOutput(behavior.update(inputAt(3030, HandDirection::Right)),
                 HorizontalAction::TurnRight, VerticalAction::Raise, BotFace::Happy);
}

void test_second_touch_restarts_release_timeout()
{
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorInput input = inputAt(100, HandDirection::None);
    input.touchPressed = true;
    input.touchPressedEvent = true;
    behavior.update(input);
    input = inputAt(200, HandDirection::None);
    input.touchReleasedEvent = true;
    behavior.update(input);

    input = inputAt(3000, HandDirection::None);
    input.touchPressed = true;
    input.touchPressedEvent = true;
    behavior.update(input);
    input = inputAt(3100, HandDirection::None);
    input.touchReleasedEvent = true;
    behavior.update(input);

    assertOutput(behavior.update(inputAt(6099, HandDirection::None)),
                 HorizontalAction::Hold, VerticalAction::Raise, BotFace::Confused);
    assertOutput(behavior.update(inputAt(6100, HandDirection::None)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
}

void test_infrared_timeout_survives_millis_wraparound()
{
    BotBehavior behavior;
    behavior.reset(UINT32_MAX - 2000U);

    behavior.update(inputAt(UINT32_MAX - 1000U, HandDirection::Left));
    behavior.update(inputAt(UINT32_MAX - 500U, HandDirection::None));

    assertOutput(behavior.update(inputAt(2498U, HandDirection::None)),
                 HorizontalAction::Hold, VerticalAction::Hold, BotFace::Confused);
    assertOutput(behavior.update(inputAt(2499U, HandDirection::None)),
                 HorizontalAction::Center, VerticalAction::Center, BotFace::Default);
}

void test_touch_filter_debounces_both_edges_and_reports_long_press_once()
{
    TouchFilter filter;
    filter.reset(0, false);

    TEST_ASSERT_FALSE(filter.update(10, true).pressed);
    TEST_ASSERT_FALSE(filter.update(109, true).pressedEvent);
    TouchEvents events = filter.update(110, true);
    TEST_ASSERT_TRUE(events.pressed);
    TEST_ASSERT_TRUE(events.pressedEvent);

    TEST_ASSERT_FALSE(filter.update(909, true).longPressEvent);
    events = filter.update(910, true);
    TEST_ASSERT_TRUE(events.longPressEvent);
    TEST_ASSERT_FALSE(filter.update(911, true).longPressEvent);

    TEST_ASSERT_FALSE(filter.update(920, false).releasedEvent);
    TEST_ASSERT_FALSE(filter.update(1019, false).releasedEvent);
    events = filter.update(1020, false);
    TEST_ASSERT_FALSE(events.pressed);
    TEST_ASSERT_TRUE(events.releasedEvent);
}

void test_touch_filter_does_not_turn_a_releasing_short_press_into_long_press()
{
    TouchFilter filter;
    filter.reset(0, false);
    filter.update(0, true);
    filter.update(100, true);

    TEST_ASSERT_FALSE(filter.update(850, false).longPressEvent);
    TEST_ASSERT_FALSE(filter.update(900, false).longPressEvent);
    TEST_ASSERT_TRUE(filter.update(950, false).releasedEvent);
}

void test_touch_filter_uses_raw_falling_edge_for_exact_long_press_boundary()
{
    TouchFilter beforeBoundary;
    beforeBoundary.reset(0, false);
    beforeBoundary.update(0, true);
    beforeBoundary.update(100, true);
    TEST_ASSERT_FALSE(beforeBoundary.update(899, false).longPressEvent);

    TouchFilter atBoundary;
    atBoundary.reset(0, false);
    atBoundary.update(0, true);
    atBoundary.update(100, true);
    TEST_ASSERT_TRUE(atBoundary.update(900, false).longPressEvent);
    TEST_ASSERT_FALSE(atBoundary.update(901, false).longPressEvent);
}

HandDirection filteredDirection(const bool leftSamples[5], const bool rightSamples[5])
{
    InfraredFilter filter;
    filter.reset();
    HandDirection direction = HandDirection::None;
    for (int index = 0; index < 5; ++index) {
        const bool completed = filter.addSample(leftSamples[index], rightSamples[index], direction);
        TEST_ASSERT_EQUAL(index == 4, completed);
    }
    return direction;
}

void test_infrared_filter_uses_four_of_five_votes_for_all_directions()
{
    const bool fourActive[5] = {true, true, false, true, true};
    const bool threeActive[5] = {true, false, true, false, true};
    const bool inactive[5] = {false, false, false, false, false};

    TEST_ASSERT_EQUAL_INT(static_cast<int>(HandDirection::Left),
                          static_cast<int>(filteredDirection(fourActive, inactive)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HandDirection::Right),
                          static_cast<int>(filteredDirection(inactive, fourActive)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HandDirection::Both),
                          static_cast<int>(filteredDirection(fourActive, fourActive)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HandDirection::None),
                          static_cast<int>(filteredDirection(threeActive, threeActive)));
}

BehaviorInput inputFromReading(uint32_t now, const InfraredReading &reading)
{
    BehaviorInput input = inputAt(now, reading.direction);
    input.infraredSampleComplete = reading.sampleWindowComplete;
    return input;
}

void test_partial_infrared_window_before_touch_release_cannot_rearm_tracking()
{
    InfraredSampler sampler;
    sampler.reset(0);
    BotBehavior behavior;
    behavior.reset(0);

    BehaviorInput touch = inputAt(1, HandDirection::None);
    touch.touchPressed = true;
    touch.touchPressedEvent = true;
    behavior.update(touch);

    for (uint32_t now = 2; now <= 8; now += 2) {
        TEST_ASSERT_FALSE(sampler.update(now, false, true).sampleWindowComplete);
    }

    // 稳定松手时丢弃触摸前未完成的 4/5 采样窗口。
    sampler.reset(8);
    InfraredReading reading = sampler.update(8, false, true);
    BehaviorInput release = inputFromReading(8, reading);
    release.touchReleasedEvent = true;
    assertOutput(behavior.update(release), HorizontalAction::Hold,
                 VerticalAction::Raise, BotFace::Confused);

    // 触摸前已经存在的右侧遮挡，即便随后形成完整窗口，也不能接手。
    for (uint32_t now = 10; now <= 18; now += 2) {
        reading = sampler.update(now, false, true);
        const BehaviorOutput output = behavior.update(inputFromReading(now, reading));
        assertOutput(output, HorizontalAction::Hold,
                     VerticalAction::Raise, BotFace::Confused);
    }
    TEST_ASSERT_TRUE(reading.sampleWindowComplete);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HandDirection::Right),
                          static_cast<int>(reading.direction));

    // 先完成一整个无遮挡窗口，再出现新的左侧遮挡，才恢复跟随。
    for (uint32_t now = 20; now <= 28; now += 2) {
        reading = sampler.update(now, false, false);
        behavior.update(inputFromReading(now, reading));
    }
    TEST_ASSERT_TRUE(reading.sampleWindowComplete);
    for (uint32_t now = 30; now <= 36; now += 2) {
        reading = sampler.update(now, true, false);
        assertOutput(behavior.update(inputFromReading(now, reading)),
                     HorizontalAction::Hold, VerticalAction::Raise, BotFace::Confused);
    }
    reading = sampler.update(38, true, false);
    assertOutput(behavior.update(inputFromReading(38, reading)),
                 HorizontalAction::TurnLeft, VerticalAction::Raise, BotFace::Happy);
}

void test_touch_filter_detects_touch_already_present_at_startup()
{
    TouchFilter filter;
    filter.reset(0, true);
    TEST_ASSERT_FALSE(filter.update(99, true).pressedEvent);
    const TouchEvents events = filter.update(100, true);
    TEST_ASSERT_TRUE(events.pressed);
    TEST_ASSERT_TRUE(events.pressedEvent);
}

void test_left_tracking_reverses_immediately_while_returning_to_center()
{
    ServoMotion motion;
    motion.reset(0);

    uint32_t now = 0;
    for (int step = 0; step < 70; ++step) {
        motion.turnLeft();
        now += 20;
        motion.update(now);
    }
    TEST_ASSERT_EQUAL_INT(20, motion.currentHorizontalAngle());

    motion.returnToCenter();
    now += 20;
    motion.update(now);
    TEST_ASSERT_EQUAL_INT(21, motion.currentHorizontalAngle());

    motion.turnLeft();
    now += 20;
    motion.update(now);
    TEST_ASSERT_EQUAL_INT(20, motion.currentHorizontalAngle());
}

void test_hold_head_clears_both_old_targets()
{
    ServoMotion motion;
    motion.reset(0);
    motion.turnRight();
    motion.raiseHead();
    motion.holdHead();
    motion.update(20);

    TEST_ASSERT_EQUAL_INT(90, motion.currentHorizontalAngle());
    TEST_ASSERT_EQUAL_INT(90, motion.currentVerticalAngle());
}

} // namespace

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_touch_preempts_tracking_and_holds_horizontal);
    RUN_TEST(test_long_touch_keeps_heart_until_three_second_timeout);
    RUN_TEST(test_short_touch_requires_clear_then_new_infrared_obstruction);
    RUN_TEST(test_old_infrared_stays_blocked_after_timeout_until_clear);
    RUN_TEST(test_lost_infrared_waits_three_seconds_then_centers);
    RUN_TEST(test_second_touch_restarts_release_timeout);
    RUN_TEST(test_infrared_timeout_survives_millis_wraparound);
    RUN_TEST(test_touch_filter_debounces_both_edges_and_reports_long_press_once);
    RUN_TEST(test_touch_filter_does_not_turn_a_releasing_short_press_into_long_press);
    RUN_TEST(test_touch_filter_uses_raw_falling_edge_for_exact_long_press_boundary);
    RUN_TEST(test_infrared_filter_uses_four_of_five_votes_for_all_directions);
    RUN_TEST(test_partial_infrared_window_before_touch_release_cannot_rearm_tracking);
    RUN_TEST(test_touch_filter_detects_touch_already_present_at_startup);
    RUN_TEST(test_left_tracking_reverses_immediately_while_returning_to_center);
    RUN_TEST(test_hold_head_clears_both_old_targets);
    return UNITY_END();
}
