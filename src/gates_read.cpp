#include "Arduino.h"
#include "gates_read.h"
#include "config.h"

// Shared variables between cores - must be volatile
volatile bool gateHigh[4] = {false, false, false, false};
volatile bool gateHighProcessed[4] = {false, false, false, false};
//olatile boolean gateLow[4] = {true, true, true, true};
volatile int lastGateState[4] = {HIGH, HIGH, HIGH, HIGH};

bool trigger_on[4] = {false, false, false, false}; // simple bool to switch trigger on and off per channel

void setupGates()
{
  // Initialise gate pins
  pinMode(GATE_1_PIN, INPUT_PULLUP);
  pinMode(GATE_2_PIN, INPUT_PULLUP);
  pinMode(GATE_3_PIN, INPUT_PULLUP);
  pinMode(GATE_4_PIN, INPUT_PULLUP);
}

void gatesUpdate()
{
  for (int ch = 0; ch < 4; ch++)
  {
    if (trigger_on[ch] == false && !checkGates(ch + 1))
    {
      trigger_on[ch] = true;
      adsr_class[ch].note_on();
      // Serial.println("Gate HIGH");
    }
    else if (trigger_on[ch] == true && checkGates(ch + 1))
    {
      trigger_on[ch] = false;
      adsr_class[ch].note_off();
      // Serial.println("Gate LOW");
    }
  }
}

bool checkGates(int gateIndex)
{
  // Check gate input using debounce with state tracking
  int gateState;
  switch (gateIndex)
  {
  case 1:
    gateState = !digitalRead(GATE_1_PIN);
    break;
  case 2:
    gateState = !digitalRead(GATE_2_PIN);
    break;
  case 3:
    gateState = !digitalRead(GATE_3_PIN);
    break;
  case 4:
    gateState = !digitalRead(GATE_4_PIN);
    break;
  default:
    gateState = !digitalRead(GATE_1_PIN);
    break;
  }

  // Only update the encoder switch state if it's changed
  if (gateState == LOW && !gateHigh[gateIndex - 1])
  {
    gateHigh[gateIndex - 1] = true;
    gateHighProcessed[gateIndex - 1] = false; // Reset processed flag
  }
  else if (gateState == HIGH && gateHigh[gateIndex - 1])
  {
    gateHigh[gateIndex - 1] = false;
  }

  // Using a processed flag prevents multiple triggers from one button press
  if (gateHigh[gateIndex - 1] && !gateHighProcessed[gateIndex - 1] /* && gateLow[gateIndex - 1]*/)
  {
    return true;

    // Mark as processed and not released yet
    //gateHighProcessed[gateIndex - 1] = true;
    //gateLow[gateIndex - 1] = false;
    //oledUpdateNeeded = true;
  }
  else
  {
    return false;
    // Button has been released
    //gateLow[gateIndex - 1] = true;
    //gateHighProcessed[gateIndex - 1] = false;
  }
}
