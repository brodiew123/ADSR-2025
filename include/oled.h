#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"

extern bool oledUpdateNeeded;            // Flag to indicate if an update is needed

void oledSetup();
void oledUpdate();
void clearArea(int x, int y, int width, int height, int flash);
void drawAngleLine(int centerX, int centerY, int radius, float startAngle, float rangeDegrees, int value, int minRange, int maxRange);
void drawAngleWedge(int centerX, int centerY, int radius, float startAngle, float rangeDegrees, int lowValue, int highValue, int minRange, int maxRange);
void drawCircleWithNotches(int centerX, int centerY, int radius, float startAngleDegrees, float endAngleDegrees, int notchLength);
void handleEncoderSwitch();
void enterMenu(State newState);
int getOctave (int inputInteger);
String getNoteName(int inputInteger); // Chromatic scale only
void saveOrLoadState();
void displayMenuState();
void enterMenu();

#endif // OLED_H