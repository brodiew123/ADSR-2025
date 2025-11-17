#include "Arduino.h"
#include "buttons.h"
#include "config.h"
#include "encoder_read.h"
#include "oled.h"
#include <adsr.h> // import class

// Shared variables between cores - must be volatile
volatile ButtonState buttonState[4] = {BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED};
volatile bool manualTrigger[4] = {false, false, false, false};
volatile unsigned long prevDebounceTime[4] = {0, 0, 0, 0};
volatile int lastSwitchState[4] = {HIGH, HIGH, HIGH, HIGH}; // Track last switch state for debouncing

void setupButtons()
{
  // Initialise button pins
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);
}

void buttonsUpdate()
{
  for (int i = 1; i <= 4; i++)
  {
    checkEncoderButton(i);
  }
}

bool checkEncoderButton(int encoderIndex)
{
  // Check encoder switch using debounce with state tracking
  int switchState;
  switch (encoderIndex)
  {
  case 1:
    switchState = digitalRead(BUTTON_1_PIN);
    break;
  case 2:
    switchState = digitalRead(BUTTON_2_PIN);
    break;
  case 3:
    switchState = digitalRead(BUTTON_3_PIN);
    break;
  case 4:
    switchState = digitalRead(BUTTON_4_PIN);
    break;
  default:
    switchState = digitalRead(BUTTON_1_PIN);
    break;
  }

  // Read and debounce the switch every call. Read the pin and use prevDebounceTime/DEBOUNCE_TIME to
  // determine stability. This preserves debounce while avoiding lost presses.
  unsigned long currentMillis = millis();

  // If switch state changed, reset debounce timer
  if (switchState != lastSwitchState[encoderIndex - 1])
  {
    prevDebounceTime[encoderIndex - 1] = currentMillis;
  }

  // If the switch state has been stable for the debounce period, accept it
  if ((currentMillis - prevDebounceTime[encoderIndex - 1]) > DEBOUNCE_TIME)
  {
    if (switchState == LOW && buttonState[encoderIndex - 1] == BUTTON_RELEASED)
    {
      buttonState[encoderIndex - 1] = BUTTON_PRESSED;
      if (channel_selected == encoderIndex)
      {
        adsr_class[channel_selected - 1].note_on();
        manualTrigger[channel_selected - 1] = true;
      }
      else
      {
        encoder_button_pressed(encoderIndex);
      }
      oledUpdateNeeded = true;
      return true;
    }
    else if (switchState == HIGH && buttonState[encoderIndex - 1] == BUTTON_PRESSED)
    {
      buttonState[encoderIndex - 1] = BUTTON_RELEASED;
      if (manualTrigger[encoderIndex - 1])
      {
        adsr_class[encoderIndex - 1].note_off();
        manualTrigger[encoderIndex - 1] = false;
      }
      return false;
    }
  }

  // Save the current state for next comparison
  lastSwitchState[encoderIndex - 1] = switchState;

  return false;
}

void encoder_button_pressed(int encoderIndex)
{
  // Prevent multiple calls - only switch if we're actually changing channels
  if (channel_selected == encoderIndex)
  {
    return;
  }

  encoderDoublePressCheck(); // Check for double press action

  Serial.print("Switching from channel ");
  Serial.print(channel_selected);
  Serial.print(" to channel ");
  Serial.println(encoderIndex);
  Serial.print("Saving current values: A=");
  Serial.print(String(getTargetValue(0, channel_selected - 1)));
  Serial.print(" D=");
  Serial.print(String(getTargetValue(1, channel_selected - 1)));
  Serial.print(" S=");
  Serial.print(String(getTargetValue(2, channel_selected - 1)));
  Serial.print(" R=");
  Serial.println(String(getTargetValue(3, channel_selected - 1)));

  // Switch to new channel
  channel_selected = encoderIndex;
  int new_ch = channel_selected - 1;

  // Reset encoder state for all 4 parameters to sync with new channel's stored values
  // The targetValue array already maintains independent state per channel
  // setTargetValue resets lastEncoderValue to -1, forcing encoder to resync
  for (int param = 0; param < 4; param++)
  {
    setTargetValue(getTargetValue(param, new_ch), param);
  }

  oledUpdateNeeded = true; // Flag to update OLED display
}

void encoderDoublePressCheck()
{
  int pressedCount = 0;
  String pressedButtons = "";
  for (int i = 0; i < 4; i++)
  {
    if (buttonState[i] == BUTTON_PRESSED)
    {
      pressedCount++;
      pressedButtons += String(i + 1) + " ";
    }
  }
  if (pressedCount == 2)
  {
    // Toggle currentState
    if (currentState != MENU_SCREEN)
    {
      enterMenu();
      Serial.println("Entering Menu Screen due to double press.");
    }
    else
    {
      currentState = ADSR_SCREEN;
      Serial.println("Exiting Menu Screen to Parameters due to double press.");
    }

    Serial.print("Two encoder buttons pressed: ");
    Serial.println(pressedButtons);
  }
  else
  {
    Serial.print("Not exactly two buttons pressed: ");
    Serial.println(pressedButtons);
  }
}