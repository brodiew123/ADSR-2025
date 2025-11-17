#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <U8g2lib.h>

// Use volatile for variables shared between cores
extern volatile bool encoderSwitchPressed; // Track the state of the encoder switch
extern volatile bool encoderSwitchPressedProcessed; // Flag to prevent multiple triggers
extern volatile bool switchReleased;
extern volatile unsigned long prevDebounceTime;
//#define DEBOUNCE_TIME 35 // Encoder switch debounce time

void encoderSetup();
void encoderUpdate();

void getEncoder1Position(int *position, int *step);
void getEncoder2Position(int *position, int *step);
void getEncoder3Position(int *position, int *step);
void getEncoder4Position(int *position, int *step);
void getEncoder1Speed(int *speed);
void getEncoder2Speed(int *speed);
void getEncoder3Speed(int *speed);
void getEncoder4Speed(int *speed);

// Backward compatibility - these now refer to encoder1
void getEncoderPosition(int *position, int *step);
void getEncoderSpeed(int *speed);

void checkEncoderSwitch();

#endif // ENCODER_H