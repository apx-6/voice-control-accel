#include "led_control.h"

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void initLEDs() {
    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);

    pinMode(BIG_LIGHT_PIN, OUTPUT);
    ledcAttach(BIG_LIGHT_PIN, 5000, 8);
    ledcWrite(BIG_LIGHT_PIN, 0);

    clearArrows();
}

// 8x32 屏幕，32列 x 8行
#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT  8

void drawRightArrow() {
    const bool pattern[8][15] = {
        {0,0,0,0,1,1,1,0,0,0,0,0,1,1,1},
        {0,0,0,1,1,1,0,0,0,0,0,1,1,1,0},
        {0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {1,1,1,1,0,0,0,0,1,1,1,1,0,0,0},
        {1,1,1,1,0,0,0,0,1,1,1,1,0,0,0},
        {0,1,1,1,0,0,0,0,0,1,1,1,0,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0},
        {0,0,0,1,1,1,1,0,0,0,0,1,1,1,1}
    };

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            if (pattern[row][col]) {
                uint16_t idx = XY(col + 1, row); // 左侧
                strip.setPixelColor(idx, strip.Color(255, 200, 0));
            }
        }
    }
    strip.show();
}

void drawLeftArrow() {
    const bool pattern[8][15] = {
        {1,1,1,0,0,0,0,0,1,1,1,0,0,0,0},
        {0,1,1,1,0,0,0,0,0,1,1,1,0,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0},
        {0,0,0,1,1,1,1,0,0,0,0,1,1,1,1},
        {0,0,0,1,1,1,1,0,0,0,0,1,1,1,1},
        {0,0,0,1,1,1,0,0,0,0,0,1,1,1,0},
        {0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {1,1,1,1,1,0,0,0,1,1,1,1,1,0,0}
    };

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            if (pattern[row][col]) {
                uint16_t idx = XY(col + 16, row); // 右侧
                strip.setPixelColor(idx, strip.Color(255, 200, 0));
            }
        }
    }
    strip.show();
}

void drawArrow(bool isLeft) {
    if (isLeft) {
        drawLeftArrow();
    } else {
        drawRightArrow();
    }
}

void clearArrows() {
    strip.clear();
    strip.show();
}

// 蛇形坐标映射（与你的原有函数一致）
uint16_t XY(uint8_t x, uint8_t y) {
    if (x % 2 == 0) {
        return (x * 8) + y;
    } else {
        return (x * 8) + (7 - y);
    }
}

void setHeadlight(uint8_t brightness) {
    brightness = constrain(brightness, 0, 255);
    ledcWrite(BIG_LIGHT_PIN, brightness);
}