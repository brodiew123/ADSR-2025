#include <SPI.h>
#include "dac.h"
#include "config.h"

// Define pins and constants
const int DAC_CHANNEL_A = 0; // Channel A constant
const int DAC_CHANNEL_B = 1; // Channel B constant

// Operating parameters
// const float VOLTAGE_MAX = 5.0;  // Maximum output voltage

// float currentVoltage = 0.0;     // Tracks current voltage in the ramp

float channelGain[4] = {0.98, 0.98, 0.98, 0.98}; // Gain for each channel, to calibrate notes over the whole range (A, B, C, D)
float channelOffsets[4] = {0, 0, 0, 0};          // Offset integers for each channel in DAC values, to calibrate zero volts (A, B, C, D)

// Stored DAC values for each channel, ready for DAC output
int dacValues[4] = {0, 0, 0, 0};
bool dacUpdateNeeded = false; // Flag to indicate if DAC update is needed

void setupDAC()
{
            Serial.println("DAC initialising"); // Add this for debugging

    pinMode(DAC_CS_PIN, OUTPUT);
    digitalWrite(DAC_CS_PIN, HIGH); // Deselect DAC initially
    pinMode(DAC_CS_PIN2, OUTPUT);
    digitalWrite(DAC_CS_PIN2, HIGH); // Deselect second DAC initially

        Serial.println("DAC initialised"); // Add this for debugging

}

// Send value to MCP4922
void dacWrite()
{
    for (int dac = 0; dac < 2; dac++)
    {
        int cs_pin = (dac == 0) ? DAC_CS_PIN : DAC_CS_PIN2;
        for (int ch = 0; ch < 2; ch++)
        {
            // Separate messages for first and second DAC
            int value;
            if (dac == 0)
            {
                value = dacValues[ch];

                if (ch == 0)
                {
                   // Serial.print(" Value: ");
                   // Serial.println(value);
                }
            }
            else
            {
                value = dacValues[ch + 2];
            }

            value = constrain(value, 0, 4095); // Ensure 12-bit range

            byte command = (ch == DAC_CHANNEL_A) ? 0x30 : 0xB0;
            // 0x30 = 0011 0000: channel A, buffered, gain=1, active
            // 0xB0 = 1011 0000: channel B, buffered, gain=1, active

            SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
            digitalWrite(cs_pin, LOW);
            SPI.transfer(command | ((value >> 8) & 0x0F)); // High byte with command
            SPI.transfer(value & 0xFF);                    // Low byte
            digitalWrite(cs_pin, HIGH);
            SPI.endTransaction();

            /*
            Serial.print("DAC ");
            Serial.print(dac +1);
            Serial.print(" Channel ");
            Serial.print((ch == DAC_CHANNEL_A) ? "A" : "B");
            Serial.print(" Value: ");
            Serial.print(value);
            Serial.print(" at micros: ");
            Serial.print(micros());
            Serial.println();
            */
        }
    }
}

// Set specific voltage (5V reference)
void cacheDacValue(int channel, float value)
{
    // Apply gain and offset for calibration
    value = (value * channelGain[channel]) + channelOffsets[channel];

    // Store the current 12 bit value to the appropriate channel
    // instead of writing to the DAC directly
    dacValues[channel] = value;

    dacUpdateNeeded = true; // Set flag to indicate DAC update is needed
}

void printDacValues()
{
    // static int lastPrintedCursorPos[4] = {-1, -1, -1, -1};
    // for (int i = 0; i < 4; i++)
    //{
    //  only print channel if cursorPos is different from last printed value
    // if (cursorPos[i] != lastPrintedCursorPos[i])
    //{
    //         if (i == 0)
    //        {
    //  Serial.print("DAC Values: ");
    //       }
    //   Serial.println();
    // Serial.print("Channel ");
    //  Serial.print(String(i + 1));
    //  Serial.print(" - ");
    //   Serial.print("cursorPos: ");
    //  Serial.print(String(cursorPos[i]));
    int i = 0;
    Serial.print(" | DAC: ");
    Serial.print(String(dacValues[i]));
    // if (i < 3)
    //  {
    //      Serial.print(", ");
    //   }
    //  else
    //  {
    Serial.println();
    //  }
    //  lastPrintedCursorPos[i] = cursorPos[i];
    // }
    //}
}
