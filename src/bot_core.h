#ifndef BOT_CORE_H
#define BOT_CORE_H

// 初始化全部硬件，并把机器人置于双舵机 90°、默认表情状态。
void initializeBot();

// 读取一次传感器、计算行为、提交舵机目标并刷新 OLED 动画。
void updateBot();

#endif
