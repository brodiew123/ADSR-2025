#include "encoder_read.h"
#include "Arduino.h"
#include "encoder.h"
#include "config.h"
#include <adsr.h> // import class

// CALIBRATION

// Encoder
int upperSpeedThreshold = 1000;
int lowerSpeedThreshold = 300;

// Variables to track the potentiometer and target value
uint16_t encoderValue = 0;

const int time_upper = 1000;
const int sustain_upper = 100;

const long long adsr_attack_min = 1000;  // minimum time in µs
const long long adsr_decay_min = 1000;   // minimum time in µs
const int adsr_sustain_min = 1;          // minimum sustain level
const long long adsr_release_min = 1000; // minimum time in µs

// Global arrays for encoder state
int initTargetValue[4][4] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};      // Initial target position for each encoder, per channel
int16_t lastEncoderValue[4][4] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}}; // Previous potentiometer reading
int16_t targetValue[4][4];
/* = {{(int16_t)(time_upper * log((double)adsr_attack / adsr_attack_min) / log((double)adsr_attack_max / adsr_attack_min)),
                          (int16_t)(time_upper * log((double)adsr_decay / adsr_decay_min) / log((double)adsr_decay_max / adsr_decay_min)),
                          (int16_t)((double)adsr_sustain / adsr_sustain_max * sustain_upper),
                          (int16_t)(time_upper * log((double)adsr_release / adsr_release_min) / log((double)adsr_release_max / adsr_release_min))},
                          {(int16_t)(time_upper * log((double)adsr_attack / adsr_attack_min) / log((double)adsr_attack_max / adsr_attack_min)),
                          (int16_t)(time_upper * log((double)adsr_decay / adsr_decay_min) / log((double)adsr_decay_max / adsr_decay_min)),
                          (int16_t)((double)adsr_sustain / adsr_sustain_max * sustain_upper),
                          (int16_t)(time_upper * log((double)adsr_release / adsr_release_min) / log((double)adsr_release_max / adsr_release_min))},
                          {(int16_t)(time_upper * log((double)adsr_attack / adsr_attack_min) / log((double)adsr_attack_max / adsr_attack_min)),
                          (int16_t)(time_upper * log((double)adsr_decay / adsr_decay_min) / log((double)adsr_decay_max / adsr_decay_min)),
                          (int16_t)((double)adsr_sustain / adsr_sustain_max * sustain_upper),
                          (int16_t)(time_upper * log((double)adsr_release / adsr_release_min) / log((double)adsr_release_max / adsr_release_min))},
                          {(int16_t)(time_upper * log((double)adsr_attack / adsr_attack_min) / log((double)adsr_attack_max / adsr_attack_min)),
                          (int16_t)(time_upper * log((double)adsr_decay / adsr_decay_min) / log((double)adsr_decay_max / adsr_decay_min)),
                          (int16_t)((double)adsr_sustain / adsr_sustain_max * sustain_upper),
                          (int16_t)(time_upper * log((double)adsr_release / adsr_release_min) / log((double)adsr_release_max / adsr_release_min))}}; // Current target value for each encoder
*/
int16_t encoderChange[4] = {0, 0, 0, 0}; // How much the potentiometer moved

// Set to true to print the potentiometer values
bool serialPrintEncoder = true;

const int CENTER_OFFSET = 32768; // Half of a 16-bit integer range

// Variables direction changes in encoder
int direction = 0; // Direction of the last change
// int lastDirection = 0; // Last direction of the change

// volatile bool arrayNewTargetValue; // Flag to indicate if the target value has changed, only for arrays

const long long adsr_attack_max = 100000000;   // time in µs
const long long adsr_decay_max = 100000000;    // time in µs
const int adsr_sustain_max = 4095;             // sustain level -> from 0 to DACSIZE-1
const long long adsr_release_max = 1000000000; // time in µs

void setupEncoderRead()
{
  // Initialise all 4 parameter positions for this channel from current ADSR globals
  for (int i = 0; i < 4; i++)
  {
    targetValue[0][i] = (int16_t)(time_upper * log((double)adsr_attack[i] / adsr_attack_min) / log((double)adsr_attack_max / adsr_attack_min));
    targetValue[1][i] = (int16_t)(time_upper * log((double)adsr_decay[i] / adsr_decay_min) / log((double)adsr_decay_max / adsr_decay_min));
    targetValue[2][i] = (int16_t)((double)adsr_sustain[i] / adsr_sustain_max * sustain_upper);
    targetValue[3][i] = (int16_t)(time_upper * log((double)adsr_release[i] / adsr_release_min) / log((double)adsr_release_max / adsr_release_min));
  }
}

int readEncoder(int lowerRange, int upperRange, int gainMax, int encoderId, int channel)
{
  // Get index for arrays (encoderId 1 -> index 0, encoderId 2 -> index 1)
  int idx = encoderId - 1;
  if (idx < 0 || idx > 3)
    idx = 0; // Default to first encoder

  int ch = channel - 1;
  if (ch < 0 || ch > 3)
    ch = 0; // Default to first channel

  // Set range based on encoder ID
  if (encoderId == 3) // Sustain
  {
    lowerRange = 0;
    upperRange = sustain_upper;
  }
  else // Attack, Decay, Release
  {
    lowerRange = 0;
    upperRange = time_upper;
  }

  // Get speed and position based on encoder ID
  int speed;
  int position, step;
  if (encoderId == 1)
  {
    getEncoder1Speed(&speed);
    getEncoder1Position(&position, &step);
  }
  else if (encoderId == 2)
  {
    getEncoder2Speed(&speed);
    getEncoder2Position(&position, &step);
  }
  else if (encoderId == 3)
  {
    getEncoder3Speed(&speed);
    getEncoder3Position(&position, &step);
  }
  else if (encoderId == 4)
  {
    getEncoder4Speed(&speed);
    getEncoder4Position(&position, &step);
  }
  else
  {
    // Default to encoder 1
    getEncoder1Speed(&speed);
    getEncoder1Position(&position, &step);
  }

  // Center the step value in a large range to avoid negative values
  int centeredStep = step + CENTER_OFFSET;
  int encoderValue = (centeredStep / 4);

  // Rolling average for speed detection using existing speed value
  const int speedSamples = 25;                    // Number of samples for rolling average
  static int speedHistory[4][speedSamples] = {0}; // Recent speed values per encoder
  static int speedIndex[4] = {0};                 // Current position in circular buffer per encoder
  static bool speedInitialised[4] = {false};      // Track initialization per encoder

  const int gainThreshold = lowerSpeedThreshold; // Threshold for allowing gain (based on speed values we see)
  static bool allowGain[4] = {false};            // Flag if encoder is moving fast enough per encoder

  // Initialise on first run
  if (!speedInitialised[idx])
  {
    speedInitialised[idx] = true;
    allowGain[idx] = false; // Start conservative
  }

  // Use the existing speed value (absolute value to handle negative speeds)
  int currentSpeed = abs(speed);

  // Update rolling average
  speedHistory[idx][speedIndex[idx]] = currentSpeed;
  speedIndex[idx] = (speedIndex[idx] + 1) % speedSamples;

  // Calculate rolling average
  int totalSpeed = 0;
  for (int i = 0; i < speedSamples; i++)
  {
    totalSpeed += speedHistory[idx][i];
  }
  int averageSpeed = totalSpeed / speedSamples;

  // Allow gain if rolling average is above threshold
  allowGain[idx] = (averageSpeed >= gainThreshold);

  // Calculate GAIN based on speed
  int gain = 1; // initialise gain variable
  if ((averageSpeed > upperSpeedThreshold || averageSpeed < (upperSpeedThreshold * -1)) && allowGain[idx])
  {
    gain = gainMax; // Set gain to maximum if speed is high
  }
  else if ((averageSpeed > lowerSpeedThreshold || averageSpeed < (lowerSpeedThreshold * -1)) && allowGain[idx])
  {
    // Create a logarithmic gain based on speed - higher speeds get exponentially more gain
    float normalisedSpeed = (float)(abs(averageSpeed) - lowerSpeedThreshold) / (upperSpeedThreshold - lowerSpeedThreshold);
    normalisedSpeed = constrain(normalisedSpeed, 0.0, 1.0); // Ensure it's between 0 and 1

    // Apply logarithmic curve: gain = 1 + (gainMax-1) * (normalisedSpeed^2)
    // This gives gentle increases at low speeds, dramatic increases at high speeds
    float gainCurve = normalisedSpeed * normalisedSpeed; // Quadratic curve (similar effect to log)
    gain = 1 + (gainMax - 1) * gainCurve;
  }
  else
  {
    gain = 1; // Minimum gain when speed is 0
  }

  /*
  // Debug printing
  static unsigned long lastModePrintTime = 0;
  if (millis() - lastModePrintTime > 100)
  {
    Serial.print(" | speed: ");
    Serial.print(String(speed));
    Serial.print(" | currentSpeed: ");
    Serial.print(String(currentSpeed));
    Serial.print(" | avgSpeed: ");
    Serial.print(String(averageSpeed));
    Serial.print(" | allowGain: ");
    Serial.print(String(allowGain));
    Serial.println();

    lastModePrintTime = millis();
  }*/

  // First reading
  if (lastEncoderValue[idx][ch] == -1)
  {
    lastEncoderValue[idx][ch] = encoderValue; // No additional offset
    initTargetValue[idx][ch] = targetValue[idx][ch];

    return targetValue[idx][ch]; // Skip first adjustment cycle
  }

  // Calculate the change in raw encoder value
  int rawEncoderChange = encoderValue - lastEncoderValue[idx][ch];
  // oledUpdateNeeded = true; // Set flag to update OLED display

  // Apply gain to the absolute change, then restore the sign
  // This ensures gain affects both positive and negative movements equally
  if (rawEncoderChange != 0)
  {
    direction = (rawEncoderChange > 0) ? 1 : -1;
    int absChange = abs(rawEncoderChange);
    encoderChange[idx] = (absChange * gain) * direction;
  }
  else
  {
    encoderChange[idx] = 0;
  }

  // Ensure minimum change for precision
  if (encoderId != 3) // Time parameters
  {
    unsigned long current = (encoderId == 1 ? adsr_attack[ch] : encoderId == 2 ? adsr_decay[ch]
                                                                               : adsr_release[ch]);
    double ratio = (double)adsr_attack_max / adsr_attack_min;
    double log_ratio = log(ratio);
    double base_delta_d = upperRange * log(1 + 10000.0 / current) / log_ratio;
    int base_delta = (int)base_delta_d;
    int max_delta = upperRange; // Allow full range for quick movement to max
    int sign = (encoderChange[idx] > 0 ? 1 : -1);
    if (abs(encoderChange[idx]) < base_delta && encoderChange[idx] != 0)
    {
      encoderChange[idx] = base_delta * sign;
    }
    if (abs(encoderChange[idx]) > max_delta)
    {
      encoderChange[idx] = max_delta * sign;
    }
  } 

   if (currentState == ADSR_SCREEN)
  {
    targetValue[idx][ch] = initTargetValue[idx][ch] + encoderChange[idx];
  }

  // Store the unquantized target value for next iteration's reference
  int unquantizedTargetValue = targetValue[idx][ch];

  switch (encoderId)
  {
  case 1:
    targetValue[0][ch] = constrain(targetValue[0][ch], lowerRange, upperRange);
    adsr_attack[ch] = (unsigned long)(adsr_attack_min * pow((double)adsr_attack_max / adsr_attack_min, (double)targetValue[0][ch] / upperRange));
    adsr_class[ch].set_attack(adsr_attack[ch]);
    break;
  case 2:
    targetValue[1][ch] = constrain(targetValue[1][ch], lowerRange, upperRange);
    adsr_decay[ch] = (unsigned long)(adsr_decay_min * pow((double)adsr_decay_max / adsr_decay_min, (double)targetValue[1][ch] / upperRange));
    adsr_class[ch].set_decay(adsr_decay[ch]);
    break;
  case 3:
    targetValue[2][ch] = constrain(targetValue[2][ch], lowerRange, upperRange);
    adsr_sustain[ch] = (unsigned long)((long long)targetValue[2][ch] * adsr_sustain_max / upperRange);
    adsr_class[ch].set_sustain(adsr_sustain[ch]);
    break;
  case 4:
    targetValue[3][ch] = constrain(targetValue[3][ch], lowerRange, upperRange);
    adsr_release[ch] = (unsigned long)(adsr_release_min * pow((double)adsr_release_max / adsr_release_min, (double)targetValue[3][ch] / upperRange));
    adsr_class[ch].set_release(adsr_release[ch]);
    break;
  default:
    // Default case (should not occur)
    break;
  }

  const long WRAP_THRESHOLD = 10000;

  // Constrain to range
  if (targetValue[idx][ch] < ((long)lowerRange))
  {
    // Value is below lower limit
    // if (activeParameter != 10) // Offset parameter wraps, so doesn't need constraint
    {
      targetValue[idx][ch] = ((long)lowerRange);
    }
  }
  else if (targetValue[idx][ch] > ((long)upperRange) && targetValue[idx][ch] < WRAP_THRESHOLD)
  {
    // Value is above upper limit but not wrapped around
    targetValue[idx][ch] = ((long)upperRange);
  }
  else if (targetValue[idx][ch] >= WRAP_THRESHOLD)
  {
    // Value is so large it must be a wrap-around error
    targetValue[idx][ch] = ((long)lowerRange);
  }

  // Update for next iteration
  lastEncoderValue[idx][ch] = encoderValue;          // No additional offset
  initTargetValue[idx][ch] = unquantizedTargetValue; // Use unquantised value as reference for next iteration

  // Apply the same constraints to initTargetValue to prevent it from going out of bounds
  if (initTargetValue[idx][ch] < ((long)lowerRange))
  {
      initTargetValue[idx][ch] = ((long)lowerRange);
  }
  else if (initTargetValue[idx][ch] > ((long)upperRange) && initTargetValue[idx][ch] < WRAP_THRESHOLD)
  {
    initTargetValue[idx][ch] = ((long)upperRange);
  }
  else if (initTargetValue[idx][ch] >= WRAP_THRESHOLD)
  {
    initTargetValue[idx][ch] = ((long)lowerRange);
  }

  if (encoderChange[idx] != 0)
  { /*
     Serial.print("Enc_");
     Serial.print(encoderId);
     Serial.print(": targetValues: Attack= ");
     Serial.print(String(targetValue[0][ch]));
     Serial.print(" Decay= ");
     Serial.print(String(targetValue[1][ch]));
     Serial.print(" Sustain= ");
     Serial.print(String(targetValue[2][ch]));
     Serial.print(" Release= ");
     Serial.print(String(targetValue[3][ch]));
     // Serial.print(" | initTargetValue = ");
     // Serial.print(String(initTargetValue[idx]));
     // Serial.print(" | encoderChange = ");
     // Serial.print(String(encoderChange[idx]));
     // Serial.print(" | gain = ");
     // Serial.print(String(gain));
     // Serial.print(" | currentChannel = ");
     // Serial.print(String(currentChannel));
     Serial.println();
 */

    // Display all values of all channels
    Serial.print("Enc_");
    Serial.print(encoderId);
    Serial.print(": ADSR Values - Ch1(A,D,S,R)=(");
    Serial.print(String(adsr_attack[0]));
    Serial.print(",");
    Serial.print(String(adsr_decay[0]));
    Serial.print(",");
    Serial.print(String(adsr_sustain[0]));
    Serial.print(",");
    Serial.println(String(adsr_release[0]));
    Serial.print(") Ch2(A,D,S,R)=(");
    Serial.print(String(adsr_attack[1]));
    Serial.print(",");
    Serial.print(String(adsr_decay[1]));
    Serial.print(",");
    Serial.print(String(adsr_sustain[1]));
    Serial.print(",");
    Serial.println(String(adsr_release[1]));
    Serial.print(") Ch3(A,D,S,R)=(");
    Serial.print(String(adsr_attack[2]));
    Serial.print(",");
    Serial.print(String(adsr_decay[2]));
    Serial.print(",");
    Serial.print(String(adsr_sustain[2]));
    Serial.print(",");
    Serial.println(String(adsr_release[2]));
    Serial.print(") Ch4(A,D,S,R)=(");
    Serial.print(String(adsr_attack[3]));
    Serial.print(",");
    Serial.print(String(adsr_decay[3]));
    Serial.print(",");
    Serial.print(String(adsr_sustain[3]));
    Serial.print(",");
    Serial.print(String(adsr_release[3]));
    Serial.println(")");
  }

  return targetValue[idx][ch];
}

void setTargetValue(int newTargetValue, int parameter)
{
  targetValue[parameter][channel_selected - 1] = newTargetValue; // Default to encoder 1
  lastEncoderValue[parameter][channel_selected - 1] = -1; // Force a fresh encoder reading
}

int getTargetValue(int parameter, int channel)
{
  return targetValue[parameter][channel]; // Default to encoder 1
}