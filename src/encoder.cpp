#include "Arduino.h"
#include "PicoEncoder.h"
//#include "oled.h"
//#include "EncoderReader.h"
#include "encoder.h"
#include "config.h"

// declare two encoders that will actually be reading the same pins:
// one will measure the phase sizes and compensate for differences, while
// the other will work as if the phase sizes were perfectly uniform
PicoEncoder encoder1;
PicoEncoder encoder1_non_calibrated;
// Second encoder
PicoEncoder encoder2;
PicoEncoder encoder2_non_calibrated;
// Third encoder
PicoEncoder encoder3;
PicoEncoder encoder3_non_calibrated;
// Fourth encoder
PicoEncoder encoder4;
PicoEncoder encoder4_non_calibrated;

// keep track of current time
uint period_start_us;

// For non-blocking timing of serial output
unsigned long lastPrintMillis = 0;
const unsigned long printInterval = 1; // Print every 

bool encoderSerialPrint = false; // Set to true to enable serial printing

int lastSpeed = -1;
/*
// Shared variables between cores - must be volatile
volatile bool encoderSwitchPressed = false;
volatile bool encoderSwitchPressedProcessed = false;
volatile unsigned long prevDebounceTime = 0;
volatile boolean switchReleased = true;
volatile int lastSwitchState = HIGH; // Track last switch state for debouncing
*/
void encoderSetup()
{
  //Serial.begin(115200);

  // Initialise encoders
  // use the same pins for both the calibrated and non-calibrated encoders
  encoder1.begin(encoder_pinA);
  encoder1_non_calibrated.begin(encoder_pinA);
  encoder2.begin(encoder2_pinA);
  encoder2_non_calibrated.begin(encoder2_pinA);
  encoder3.begin(encoder3_pinA);
  encoder3_non_calibrated.begin(encoder3_pinA);
  encoder4.begin(encoder4_pinA);
  encoder4_non_calibrated.begin(encoder4_pinA);
  
  // Configure encoder switch pin as input with pullup
  //pinMode(encoder_switch, INPUT_PULLUP);

  //delay(500);

  period_start_us = time_us_32();
}

void encoderUpdate()
{
  static int calibrationCounter = 0;
  static bool initialCalibrationDone = false;
  
  // During startup (first 100 calls), do intensive calibration
  if (!initialCalibrationDone) {
    while ((int)(time_us_32() - period_start_us) < 10000)
      encoder1.autoCalibratePhases();
    period_start_us += 10000;

    calibrationCounter++;
    if (calibrationCounter >= 100) {
      initialCalibrationDone = true;
      calibrationCounter = 0; // Reset counter for maintenance calibration
    }
  } else {
    // After startup, do maintenance calibration much less frequently
    // Only do intensive calibration every 50th call (~once every 0.5 seconds at 10ms calls)
    if (calibrationCounter % 50 == 0) {
      while ((int)(time_us_32() - period_start_us) < 10000)
        encoder1.autoCalibratePhases();
      period_start_us += 10000;
    } else {
      // Just do a quick calibration step without blocking
      encoder1.autoCalibratePhases();
      period_start_us = time_us_32(); // Reset timer to avoid accumulation
    }
    calibrationCounter++;
  }

  encoder1.update();
  encoder1_non_calibrated.update();
  encoder2.update();
  encoder2_non_calibrated.update();
  encoder3.update();
  encoder3_non_calibrated.update();
  encoder4.update();
  encoder4_non_calibrated.update();

  // Print values at regular intervals without blocking
  if (millis() - lastPrintMillis >= printInterval) {
    lastPrintMillis = millis();

 //  if (lastSpeed != encoder.speed) {
  //    Serial.print("Encoder speed: ");
  //    Serial.println(encoder.speed);
  //    lastSpeed = encoder.speed;
  //}

    if (encoderSerialPrint) {
        Serial.print("Encoder1 - speed: ");
        Serial.print(String(encoder1.speed));
        Serial.print(", position: ");
        Serial.print(String(encoder1.position));
        Serial.print(", step: ");
        Serial.print(String(encoder1.step));
        if (encoder1.autoCalibrationDone()) {
            Serial.print(", phases: 0x");
            Serial.print(String(encoder1.getPhases(), HEX));
        }
        Serial.print(", non calibrated speed: ");
        Serial.print(String(encoder1_non_calibrated.speed));
        Serial.print(" | Encoder2 - speed: ");
        Serial.print(String(encoder2.speed));
        Serial.print(", position: ");
        Serial.print(String(encoder2.position));
        Serial.print(", step: ");
        Serial.print(String(encoder2.step));
        Serial.println();
    }
  }

  /*
  // Check encoder switch using debounce with state tracking
  int switchState = expanderEncoderState; // Pull state of encoder switch from expander
  
  // Only process switch changes at a reasonable interval (every 5ms)
  static unsigned long lastSwitchCheckTime = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastSwitchCheckTime >= 5) {
    lastSwitchCheckTime = currentMillis;
    
    // If switch state changed
    if (switchState != lastSwitchState) {
      // Reset the debounce timer
      prevDebounceTime = currentMillis;
    }
    
    // If the switch state has been stable for the debounce period
    if ((currentMillis - prevDebounceTime) > DEBOUNCE_TIME) {
      // Only update the encoder switch state if it's changed
      if (switchState == LOW && !encoderSwitchPressed) {
        encoderSwitchPressed = true;
        encoderSwitchPressedProcessed = false;  // Reset processed flag
      } 
      else if (switchState == HIGH && encoderSwitchPressed) {
        encoderSwitchPressed = false;
      }
    }
    
    // Save the current state for next comparison
    lastSwitchState = switchState;
  }
*/
}

void getEncoder1Position(int *position, int *step)
{
  *position = encoder1.position;
  *step = encoder1.step;
}

void getEncoder2Position(int *position, int *step)
{
  *position = encoder2.position;
  *step = encoder2.step;
}

void getEncoder3Position(int *position, int *step)
{
  *position = encoder3.position;
  *step = encoder3.step;
}

void getEncoder4Position(int *position, int *step)
{
  *position = encoder4.position;
  *step = encoder4.step;
}

void getEncoder1Speed(int *speed)
{
  *speed = encoder1.speed;
}

void getEncoder2Speed(int *speed)
{
  *speed = encoder2.speed;
}

void getEncoder3Speed(int *speed)
{
  *speed = encoder3.speed;
}

void getEncoder4Speed(int *speed)
{
  *speed = encoder4.speed;
}

// Backward compatibility functions - these now refer to encoder1
void getEncoderPosition(int *position, int *step)
{
  getEncoder1Position(position, step);
}

void getEncoderSpeed(int *speed)
{
  getEncoder1Speed(speed);
}
/*
void checkEncoderSwitch()
{
    // Using a processed flag prevents multiple triggers from one button press
    // This is critical for multi-core applications
    if (encoderSwitchPressed && !encoderSwitchPressedProcessed && switchReleased)
    {
        // Process the button press
        //handleEncoderSwitch();
        
        // Mark as processed and not released yet
        encoderSwitchPressedProcessed = true;
        switchReleased = false;
        //oledUpdateNeeded = true;
    }
    else if (!encoderSwitchPressed && !switchReleased)
    {
        // Button has been released
        switchReleased = true;
        encoderSwitchPressedProcessed = false;
    }
}
*/