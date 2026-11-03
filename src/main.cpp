#include <Arduino.h>

#include "bot_core.h"

// Arduino 框架要求的启动回调。具体硬件初始化集中在 bot_core 中，
// 这样 main.cpp 不需要知道传感器、舵机和 OLED 的内部实现。
void setup()
{
    initializeBot();
}

// Arduino 框架会持续调用 loop()。每一轮只推进一次非阻塞状态机，
// 所有等待都由 millis() 计时，不在这里使用 delay()。
void loop()
{
    updateBot();
}
