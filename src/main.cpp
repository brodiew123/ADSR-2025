#include <adsr.h> // import class
#include <SPI.h>
#include <Arduino.h>
#include <dac.h>
#include "encoder.h"
#include "encoder_read.h"
#include "oled.h"
#include <U8g2lib.h>
#include <Wire.h>
#include "buttons.h"
#include "gates_read.h"

#define DACSIZE 4096 // vertical resolution of the DACs

const uint8_t LOWER_LIMIT = 0;
const uint16_t UPPER_LIMIT = 1000;
const uint8_t GAIN_MAX = 8;

// Default variables
unsigned long adsr_attack[4] = {100000, 100000, 100000, 100000};//100000;            // time in µs
unsigned long adsr_decay[4] = {100000, 100000, 100000, 100000};             // time in µs
int           adsr_sustain[4] = {2500, 2500, 2500, 2500};//2500;             // sustain level -> from 0 to DACSIZE-1
unsigned long adsr_release[4] = {1000000, 1000000, 1000000, 1000000};         // time in µs

// internal classes
ADSR adsr_class[4] = {ADSR(DACSIZE), ADSR(DACSIZE), ADSR(DACSIZE), ADSR(DACSIZE)}; // ADSR class initialisation, one per channel

int channel_selected = 1; // currently selected channel (1-4)

State currentState = ADSR_SCREEN; // Default state

void setup()
{
  encoderSetup();
  setupEncoderRead();
  oledSetup();

  // analogWriteResolution(12);                        // set the analog output resolution to 12 bit (4096 levels) -> ARDUINO DUE ONLY

  pinMode(LED_BUILTIN, OUTPUT); // initialize LED

  setupButtons();

  setupGates();
}

void loop()
{ 
  readEncoder(LOWER_LIMIT, UPPER_LIMIT, GAIN_MAX, 1, channel_selected); // Attack
  readEncoder(LOWER_LIMIT, UPPER_LIMIT, GAIN_MAX, 2, channel_selected); // Decay
  readEncoder(LOWER_LIMIT, UPPER_LIMIT, GAIN_MAX, 3, channel_selected); // Sustain
  readEncoder(LOWER_LIMIT, UPPER_LIMIT, GAIN_MAX, 4, channel_selected); // Release

  encoderUpdate(); 

  static unsigned long lastSlowUpdate = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastSlowUpdate >= 6)
  {
    oledUpdate();
    lastSlowUpdate = currentTime;
  }

  buttonsUpdate();

  gatesUpdate();
}

void setup1()
{
    SPI.begin();
  Serial.begin(115200);
  delay(100);

  setupDAC();
}

void loop1()
{
  for (int ch = 0; ch < 4; ch++) {
      int env_value = adsr_class[ch].envelope();
      // Serial.print("ADSR Envelope Value: ");
      // Serial.println(env_value);

      cacheDacValue(ch, env_value); // Cache DAC value for channel ch
  }
  
  dacWrite();                  // Write cached values to DAC
}
