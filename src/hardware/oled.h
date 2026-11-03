#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

enum class FaceEmotion : uint8_t {
    Default,
    Surprised,
    Happy,
    Confused,
    Heart
};

void init_Oled(Adafruit_SSD1306 &disp);
void updateFace(Adafruit_SSD1306 &disp);
void setFaceEmotion(FaceEmotion emotion);

#endif
