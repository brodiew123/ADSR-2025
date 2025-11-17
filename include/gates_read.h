
#ifndef GATES_READ_H
#define GATES_READ_H

#include <Arduino.h>
#include <SPI.h>

// Function declarations

void setupGates();
void gatesUpdate();
bool checkGates(int gateIndex);

#endif
