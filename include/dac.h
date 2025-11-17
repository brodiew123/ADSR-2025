
#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <SPI.h>

extern bool dacUpdateNeeded; // Flag to indicate if DAC update is needed

// Function declarations
void setupDAC();
void dacWrite();
void cacheDacValue(int channel, float voltage);
void printDacValues();

#endif
