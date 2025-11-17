#ifndef encoder_read_H
#define encoder_read_H

#include <Arduino.h>

// Declare the external variables so they can be accessed from main code
extern volatile bool arrayNewTargetValue; 
extern int direction;
//extern int lastDirection;

// Function declarations
void setupEncoderRead();
int readEncoder(int lowerRange, int upperRange, int gainMax, int encoderId, int channel);
void setTargetValue(int newTargetValue, int parameter);
int getTargetValue(int parameter, int channel);

#endif // encoder_read_H