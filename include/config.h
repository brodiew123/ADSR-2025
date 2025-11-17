
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <adsr.h>

// Constants
#define DEBOUNCE_TIME 35 // Encoder switch debounce time

// Public variables
extern unsigned long adsr_attack[4];            // time in µs
extern unsigned long adsr_decay[4];             // time in µs
extern int           adsr_sustain[4];                       // sustain level -> from 0 to DACSIZE-1
extern unsigned long adsr_release[4];         // time in µs

extern const long long adsr_attack_max;            // time in µs
extern const long long adsr_decay_max;             // time in µs
extern const int           adsr_sustain_max;                       // sustain level -> from 0 to DACSIZE-1
extern const long long adsr_release_max;         // time in µs

extern unsigned long trigger_duration;       // time in µs
extern unsigned long space_between_triggers; // time in µs

extern ADSR adsr_class[4];                      // ADSR class instances, one per channel

extern bool oledUpdateNeeded;            // Flag to indicate if an update is needed

extern int channel_selected;                    // currently selected channel (0-3)

// Screen states
enum State
{
  ADSR_SCREEN,
  MENU_SCREEN,
};
extern State currentState;


// PINS
// Encoder 1: must be consecutive pins on the rp2040
const int encoder_pinA = 16;
const int encoder_pinB = 17;

// Encoder 2
const int encoder2_pinA = 20;
const int encoder2_pinB = 21;

// Encoder 3
const int encoder3_pinA = 14;
const int encoder3_pinB = 15;

// Encoder 4
const int encoder4_pinA = 10;
const int encoder4_pinB = 11;

//DACs
const int DAC_CS_PIN = 22; // Chip Select pin for first DAC
const int DAC_CS_PIN2 = 27; // Chip Select pin for second DAC

// Encoder button pins
const int BUTTON_1_PIN = 6;
const int BUTTON_2_PIN = 7;
const int BUTTON_3_PIN = 8;
const int BUTTON_4_PIN = 9;

// Gate input pins
const int GATE_1_PIN = 0;
const int GATE_2_PIN = 1;
const int GATE_3_PIN = 2;
const int GATE_4_PIN = 3;

#endif