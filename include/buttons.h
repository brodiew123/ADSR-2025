
#ifndef SWITCHES_H
#define SWITCHES_H

#include <Arduino.h>
#include <SPI.h>

// Button state machine
enum ButtonState { BUTTON_RELEASED, BUTTON_PRESSED };

// Function declarations
void setupButtons();
void buttonsUpdate();
bool checkEncoderButton(int encoderIndex);
void encoder_button_pressed(int encoderIndex);
void encoderDoublePressCheck();

// Shared variables
extern volatile ButtonState buttonState[4];
extern volatile bool manualTrigger[4];

#endif