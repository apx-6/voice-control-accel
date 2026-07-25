#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN        1
#define BIG_LIGHT_PIN  4
#define NUM_LEDS       256
#define LED_BRIGHTNESS 50

extern Adafruit_NeoPixel strip;
extern uint8_t g_headlightBrightness;

void initLEDs();
void drawArrow(bool isLeft);
void clearArrows();
void setHeadlight(uint8_t brightness);
uint16_t XY(uint8_t x, uint8_t y);

#endif