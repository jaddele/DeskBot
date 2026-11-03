#include "oled.h"

#include "robot_config.h"

// 这个文件内部使用的变量和辅助函数都放进匿名命名空间，
// 避免它们和项目中其他文件的同名变量发生冲突。
namespace {

// 两只眼睛的中心坐标。屏幕大小是 128 x 64，Y=32 正好位于屏幕中间。
constexpr int16_t LEFT_EYE_X = 40;
constexpr int16_t RIGHT_EYE_X = 88;
constexpr int16_t EYE_CENTER_Y = 32;

// 切换表情时，旧表情先用 140 ms 收拢，新表情再用 140 ms 展开。
constexpr uint16_t TRANSITION_HALF_MS = 140;

// 一次完整眨眼分为：闭眼、短暂停留、睁眼三个阶段。
constexpr uint16_t BLINK_CLOSE_MS = 70;
constexpr uint16_t BLINK_HOLD_MS = 40;
constexpr uint16_t BLINK_OPEN_MS = 70;

// 每隔约 3.2 秒自动眨眼一次。
constexpr uint32_t BLINK_INTERVAL_MS = 3200;

// 普通眼睛的外形参数。
struct EyeStyle {
    int8_t offsetX;       // 相对眼睛中心的水平偏移
    int8_t offsetY;       // 相对眼睛中心的垂直偏移
    uint8_t width;        // 眼睛宽度
    uint8_t height;       // 眼睛高度
    uint8_t radius;       // 圆角半径
    int8_t topTilt;       // 上边缘倾斜量，疑惑表情会用到
};

// 一个表情由左眼和右眼两组参数组成，允许两只眼睛形状不同。
struct FacePreset {
    EyeStyle left;
    EyeStyle right;
};

// 三种左右对称的基础眼形。
constexpr EyeStyle EYE_DEFAULT = {0, 0, 40, 40, 8, 0};
constexpr EyeStyle EYE_SURPRISED = {0, 0, 44, 44, 16, 0};
constexpr EyeStyle EYE_HAPPY = {0, 0, 40, 10, 5, 0};

// 表情参数表的顺序必须和 oled.h 中 FaceEmotion 枚举的顺序一致。
constexpr FacePreset PRESETS[] = {
    {EYE_DEFAULT, EYE_DEFAULT},                         // 默认
    {EYE_SURPRISED, EYE_SURPRISED},                     // 惊讶：眼睛更大、更圆
    {EYE_HAPPY, EYE_HAPPY},                             // 开心：眼睛压成圆角细条
    {{0, -6, 40, 26, 8, 8}, EYE_DEFAULT},              // 疑惑：左眼抬高并倾斜
    {{0, 0, 34, 34, 0, 0}, {0, 0, 34, 34, 0, 0}}       // 爱心眼：这里只保存整体尺寸
};

// currentEmotion 是当前表情，targetEmotion 是准备切换到的表情。
FaceEmotion currentEmotion = FaceEmotion::Default;
FaceEmotion targetEmotion = FaceEmotion::Default;
uint32_t transitionStartedAt = 0;
bool transitionActive = false;

// 自动眨眼需要记录上次眨眼时间，以及当前眨眼的开始时间。
uint32_t lastBlinkAt = 0;
uint32_t blinkStartedAt = 0;
bool blinkActive = false;

/**
 * @brief 将表情枚举转换为表情参数表的数组下标。
 *
 * @param emotion 要转换的表情枚举值。
 * @return 对应的 PRESETS 数组下标。
 */
uint8_t emotionIndex(FaceEmotion emotion)
{
    return static_cast<uint8_t>(emotion);
}

/**
 * @brief 在指定位置绘制一只实心爱心眼。
 *
 * 两个圆构成爱心上方的圆瓣，三角形构成下方尖角。
 * scaleY 会把图形沿 Y 轴压缩，用于眨眼和表情切换动画。
 *
 * @param disp 已初始化的 OLED 显示对象。
 * @param centerX 爱心的水平中心坐标。
 * @param centerY 爱心的垂直中心坐标。
 * @param width 爱心的原始宽度，单位为像素。
 * @param height 爱心的原始高度，单位为像素。
 * @param scaleY 纵向缩放比例，范围通常为 0.0 到 1.0。
 * @return 无返回值。
 */
void drawHeart(Adafruit_SSD1306 &disp, int16_t centerX, int16_t centerY,
               uint8_t width, uint8_t height, float scaleY)
{
    // 高度最小保留 1 像素，防止闭眼时传入 0 导致图形参数异常。
    const int16_t scaledHeight = max<int16_t>(1, static_cast<int16_t>(height * scaleY));
    const int16_t halfWidth = width / 2;
    const int16_t lobeRadius = max<int16_t>(1, static_cast<int16_t>(scaledHeight * 0.28f));
    const int16_t lobeY = centerY - scaledHeight / 4;
    const int16_t lobeOffset = width / 4;
    const int16_t shoulderY = centerY - scaledHeight / 6;
    const int16_t tipY = centerY + scaledHeight / 2;

    disp.fillCircle(centerX - lobeOffset, lobeY, lobeRadius, SSD1306_WHITE);
    disp.fillCircle(centerX + lobeOffset, lobeY, lobeRadius, SSD1306_WHITE);
    disp.fillTriangle(centerX - halfWidth, shoulderY,
                      centerX + halfWidth, shoulderY,
                      centerX, tipY, SSD1306_WHITE);
}

/**
 * @brief 按 EyeStyle 参数绘制一只普通眼睛。
 *
 * 先绘制白色圆角矩形；当 topTilt 不为 0 时，再用黑色三角形切出倾斜上边缘。
 * 这套绘制方式覆盖默认、惊讶、开心和疑惑四种非爱心表情。
 *
 * @param disp 已初始化的 OLED 显示对象。
 * @param centerX 眼睛的水平中心坐标。
 * @param centerY 眼睛的垂直中心坐标。
 * @param style 当前眼睛的尺寸、偏移、圆角和倾斜参数。
 * @param scaleY 纵向缩放比例，用于眨眼和切换动画。
 * @return 无返回值。
 */
void drawStyledEye(Adafruit_SSD1306 &disp, int16_t centerX, int16_t centerY,
                   const EyeStyle &style, float scaleY)
{
    const int16_t height = max<int16_t>(1, static_cast<int16_t>(style.height * scaleY));
    const int16_t x = centerX + style.offsetX - style.width / 2;
    const int16_t y = centerY + style.offsetY - height / 2;
    const int16_t radius = min<int16_t>(style.radius, height / 2);

    disp.fillRoundRect(x, y, style.width, height, radius, SSD1306_WHITE);

    // topTilt 为正时切右上角，为负时切左上角；为 0 时保持普通圆角形状。
    const int16_t tilt = static_cast<int16_t>(style.topTilt * scaleY);
    if (tilt > 0) {
        disp.fillTriangle(x, y, x + style.width, y,
                          x + style.width, y + tilt, SSD1306_BLACK);
    } else if (tilt < 0) {
        disp.fillTriangle(x, y, x + style.width, y,
                          x, y - tilt, SSD1306_BLACK);
    }
}

/**
 * @brief 推进自动眨眼状态，并返回本帧眼睛的纵向缩放比例。
 *
 * 一次眨眼依次经历闭眼、停留和睁眼三个阶段。表情切换期间不启动新的眨眼。
 *
 * @param now 当前 millis() 时间戳，单位为毫秒。
 * @return 纵向缩放比例：1.0 表示完全睁开，0.0 表示完全闭合。
 */
float blinkScale(uint32_t now)
{
    // 表情正在切换时不触发自动眨眼，避免两个动画互相干扰。
    if (!blinkActive && !transitionActive && now - lastBlinkAt >= BLINK_INTERVAL_MS) {
        blinkActive = true;
        blinkStartedAt = now;
    }

    if (!blinkActive) {
        return 1.0f;
    }

    const uint32_t elapsed = now - blinkStartedAt;
    if (elapsed < BLINK_CLOSE_MS) {
        // 第一阶段：从 1 逐渐减小到 0，眼睛闭合。
        return 1.0f - static_cast<float>(elapsed) / BLINK_CLOSE_MS;
    }
    if (elapsed < BLINK_CLOSE_MS + BLINK_HOLD_MS) {
        // 第二阶段：保持闭眼一小会儿。
        return 0.0f;
    }
    if (elapsed < BLINK_CLOSE_MS + BLINK_HOLD_MS + BLINK_OPEN_MS) {
        // 第三阶段：从 0 逐渐增加到 1，眼睛重新睁开。
        return static_cast<float>(elapsed - BLINK_CLOSE_MS - BLINK_HOLD_MS) /
               BLINK_OPEN_MS;
    }

    // 三个阶段都结束，复位状态并重新开始计算下次眨眼间隔。
    blinkActive = false;
    lastBlinkAt = now;
    return 1.0f;
}

/**
 * @brief 推进表情切换动画，确定当前帧要绘制的表情及其缩放比例。
 *
 * 前半段绘制旧表情并逐渐收拢；后半段绘制目标表情并逐渐展开。
 *
 * @param now 当前 millis() 时间戳，单位为毫秒。
 * @param[out] scale 输出当前表情的纵向缩放比例。
 * @return 当前帧应绘制的表情枚举。
 */
FaceEmotion transitionEmotion(uint32_t now, float &scale)
{
    if (!transitionActive) {
        scale = 1.0f;
        return currentEmotion;
    }

    const uint32_t elapsed = now - transitionStartedAt;
    if (elapsed < TRANSITION_HALF_MS) {
        // 旧表情从完全展开缩小到一条线。
        scale = 1.0f - static_cast<float>(elapsed) / TRANSITION_HALF_MS;
        return currentEmotion;
    }

    if (elapsed < TRANSITION_HALF_MS * 2UL) {
        // 在最扁的位置换成新表情，再把新表情展开。
        scale = static_cast<float>(elapsed - TRANSITION_HALF_MS) /
                TRANSITION_HALF_MS;
        return targetEmotion;
    }

    // 动画结束，把目标表情正式设为当前表情。
    currentEmotion = targetEmotion;
    transitionActive = false;
    lastBlinkAt = now;
    scale = 1.0f;
    return currentEmotion;
}

} // namespace

/**
 * @brief 初始化 ESP32 的 I2C 总线和 SSD1306 OLED 显示屏。
 *
 * 本项目固定使用 SDA=21、SCL=22 和 I2C 地址 0x3C。
 * 初始化成功后，会记录自动眨眼计时的起始时间。
 *
 * @param disp main.cpp 创建并传入的 OLED 显示对象。
 * @return 无返回值；若显示屏初始化失败，程序会停在本函数内。
 */
void init_Oled(Adafruit_SSD1306 &disp)
{
    // ESP32 的 SDA 使用 21 号引脚，SCL 使用 22 号引脚。
    Wire.begin(RobotConfig::OledSdaPin, RobotConfig::OledSclPin);

    // 初始化地址为 0x3C 的 SSD1306。初始化失败时停在这里，避免继续运行。
    if (!disp.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        while (1) {
        }
    }

    disp.setTextColor(SSD1306_WHITE);
    lastBlinkAt = millis();
}

/**
 * @brief 请求切换到指定表情，并启动一次收拢再展开的切换动画。
 *
 * 重复设置当前目标表情会被忽略。若上一个切换动画尚未结束，
 * 函数会尽量从当前已显示的表情继续新的切换。
 *
 * @param emotion 希望切换到的表情枚举值。
 * @return 无返回值。
 */
void setFaceEmotion(FaceEmotion emotion)
{
    // 忽略无效枚举值，也忽略重复设置同一个目标表情的请求。
    if (emotionIndex(emotion) >= emotionIndex(FaceEmotion::Heart) + 1 ||
        emotion == targetEmotion) {
        return;
    }

    // 如果上一次切换还没结束：
    // 已进入新表情展开阶段时，就把新表情当作下一次切换的起点。
    if (transitionActive) {
        const uint32_t elapsed = millis() - transitionStartedAt;
        if (elapsed >= TRANSITION_HALF_MS) {
            currentEmotion = targetEmotion;
        }
    }

    // 保存新目标并启动一次“收拢 -> 展开”动画。
    targetEmotion = emotion;
    transitionStartedAt = millis();
    transitionActive = true;

    // 表情切换本身已经像一次眨眼，因此停止正在进行的自动眨眼。
    blinkActive = false;
}

/**
 * @brief 根据当前动画状态，将两只眼睛绘制到 OLED 的显示缓冲区。
 *
 * 本函数只负责绘制，不清屏，也不把缓冲区发送到 OLED 硬件。
 * 正常情况下由 updateFace() 调用。
 *
 * @param disp 已初始化的 OLED 显示对象。
 * @return 无返回值。
 */
void drawFace(Adafruit_SSD1306 &disp)
{
    const uint32_t now = millis();
    float transition = 1.0f;
    const FaceEmotion emotion = transitionEmotion(now, transition);

    // 切换动画和眨眼动画都作用在眼睛高度上，两者比例相乘得到最终高度。
    const float scaleY = transition * blinkScale(now);
    const FacePreset &preset = PRESETS[emotionIndex(emotion)];

    // 爱心不是圆角矩形，需要走单独的绘制函数。
    if (emotion == FaceEmotion::Heart) {
        drawHeart(disp, LEFT_EYE_X, EYE_CENTER_Y,
                  preset.left.width, preset.left.height, scaleY);
        drawHeart(disp, RIGHT_EYE_X, EYE_CENTER_Y,
                  preset.right.width, preset.right.height, scaleY);
        return;
    }

    drawStyledEye(disp, LEFT_EYE_X, EYE_CENTER_Y, preset.left, scaleY);
    drawStyledEye(disp, RIGHT_EYE_X, EYE_CENTER_Y, preset.right, scaleY);
}

/**
 * @brief 刷新一帧 OLED 表情动画。
 *
 * 每次调用会清除上一帧、计算眨眼和表情切换状态、绘制当前表情，
 * 最后把显示缓冲区发送给 OLED。应在 loop() 中持续调用。
 *
 * @param disp 已初始化的 OLED 显示对象。
 * @return 无返回值。
 */
void updateFace(Adafruit_SSD1306 &disp)
{
    disp.clearDisplay();
    drawFace(disp);
    disp.display();
}
