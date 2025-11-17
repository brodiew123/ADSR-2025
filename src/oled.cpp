#include <Arduino.h>
#include <U8g2lib.h>
#include "oled.h"
#include "encoder_read.h"
#include "encoder.h"
#include <Wire.h>
#include "adsr.h"
#include "config.h"

// Using the I2C interface for a 128x64 SSD1306 OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Font declarations
const uint8_t *mainFont = u8g2_font_tenthinguys_tr;          // 9px height font for main text
const uint8_t *smallFont = u8g2_font_boutique_bitmap_9x9_tr; // 6px height font for smaller text

// Variables
unsigned long previousMillis = 0;
const unsigned long refreshInterval = 50; // Update interval in ms
int lastTargetValue = -1;                 // To track the last target value
int lastPositionValue = -1;               // To track the last encoder position value
bool oledUpdateNeeded = false;            // Flag to indicate if an update is needed
uint8_t lastDisplayedMode = 255;          // To track the last mode displayed on OLED (initialise to invalid value)
bool popupActive = false;                 // Flag to indicate if a popup is currently being displayed

bool oledSerialPrint = false; // Set to true to enable serial printing

// Menu variables
int selectedChannel = 0;
int selectedParameter = 0;
int highlightedValue = 0;
int encoderPos = 0;
int lastEncoderPos = 0;
int maxValue;
unsigned long lastOLEDUpdate = 0;

// Encoder variables to replace with the actual encoder
int position, step;

// Save and load functionality
bool deletingFile = false; // True if deleting a file, false if saving or loading

// Copy and paste functionality
bool copyRhythm = true; // Copy rhythm flag
bool copyCvOut = true;
bool copyGateLength = true;
bool copyGlide = true;
bool copyProbability = true;
bool copyRoll = true;

// Menu arrays
const int numMenuItems = 8;
char menuItemNames[8][20] = {"Save", "Load", "blank", "blank", "blank", "blank", "blank", "blank"};

void oledSetup()
{
    delay(100); // Show test message for 2 seconds

    u8g2.begin();
    u8g2.setFont(smallFont); // Set default font at initialization
    u8g2.clear();

    u8g2.setFontMode(1); // Transparent
    u8g2.setDrawColor(1);
    u8g2.setFontDirection(0);
    u8g2.sendBuffer();
}

// Clear a specific area on the display
void clearArea(int x, int y, int width, int height, int flash)
{
    // SHOW THE AREA THAT WILL BE CLEARED
    if (flash)
    {
        u8g2.setDrawColor(1);                           // Set to white
        u8g2.drawBox(x, y - height + 1, width, height); // Draw white box over the area
        u8g2.sendBuffer();
    }

    u8g2.setDrawColor(0);                           // Set to black (clear)
    u8g2.drawBox(x, y - height + 1, width, height); // Draw black box over the area
    u8g2.setDrawColor(1);                           // Set back to white for drawing
}

// Helper function to draw a line at a specific angle based on a value within a range
void drawAngleLine(int centerX, int centerY, int radius, float startAngle, float rangeDegrees,
                   int value, int minRange, int maxRange)
{
    // Calculate the angle based on value's position within its range
    float valuePercentage = 0;
    if (maxRange > minRange)
    {
        valuePercentage = (float)(value - minRange) / (maxRange - minRange);
        valuePercentage = constrain(valuePercentage, 0.0, 1.0); // Ensure it's in 0-1 range
    }

    // Map the percentage to the angle range
    float mappedAngle = startAngle + (valuePercentage * rangeDegrees);

    // Normalize the angle to 0-359°
    while (mappedAngle < 0)
        mappedAngle += 360.0;
    while (mappedAngle >= 360.0)
        mappedAngle -= 360.0;

    // Convert to radians
    float angleRad = mappedAngle * 3.14159 / 180.0;

    // Calculate endpoint of the line
    int endX = centerX + round(radius * cos(angleRad));
    int endY = centerY + round(radius * sin(angleRad));

    // Draw the line from center to edge
    u8g2.drawLine(centerX, centerY, endX, endY);
}

void displayParametersState()
{
    int ch = channel_selected - 1; // 0-based channel index
    
    // Define graph area
    const int GRAPH_X = 10;
    const int GRAPH_Y = 10;
    const int GRAPH_WIDTH = 108;
    const int GRAPH_HEIGHT = 35;

    int release_div = 10; // Scale down release for display
    unsigned long long mapped_release = adsr_release[ch] / release_div; // Scale down release for display
    unsigned long long max_mapped_release = adsr_release_max / release_div;

    // Calculate total time for scaling
    unsigned long sustain_time = (adsr_attack_max + adsr_decay_max + max_mapped_release) / 10;
    long long total_setting_time = (long long)adsr_attack_max + (long long)adsr_decay_max + (long long)sustain_time + (long long)max_mapped_release;  // Use long long to prevent overflow
    long long total_time = (long long)adsr_attack[ch] + (long long)adsr_decay[ch] + (long long)sustain_time + (long long)mapped_release;
    
    // Fit screen to size of pulse
    float scale_factor = (float)total_setting_time / total_time;
    if (scale_factor > 1000.0f) scale_factor = 1000.0f;
    total_setting_time = (long long)((float)total_setting_time / scale_factor); 
    
    if (total_time == 0)
        total_time = 1; // Avoid division by zero

    // Calculate x positions for each phase (use long long for sums to avoid overflow)
    long long cumulative_attack = (long long)adsr_attack[ch];
    long long cumulative_decay = (long long)adsr_attack[ch] + (long long)adsr_decay[ch];
    long long cumulative_sustain_end = (long long)adsr_attack[ch] + (long long)adsr_decay[ch] + (long long)sustain_time;
    long long cumulative_release = (long long)adsr_attack[ch] + (long long)adsr_decay[ch] + (long long)sustain_time + (long long)mapped_release;

    int x_attack = GRAPH_X + (int)((cumulative_attack * (long long)GRAPH_WIDTH) / total_setting_time);
    int x_decay = GRAPH_X + (int)((cumulative_decay * (long long)GRAPH_WIDTH) / total_setting_time);
    int x_sustain_end = GRAPH_X + (int)((cumulative_sustain_end * (long long)GRAPH_WIDTH) / total_setting_time);
    int x_release = GRAPH_X + (int)((cumulative_release * (long long)GRAPH_WIDTH) / total_setting_time);

    // Calculate y positions
    int y_max = GRAPH_Y + GRAPH_HEIGHT;
    int y_min = GRAPH_Y;
    int y_sustain = GRAPH_Y + GRAPH_HEIGHT - (adsr_sustain[ch] * GRAPH_HEIGHT) / 4095;

    // Show active channel
        // Calculate width of the number + "ms"
    char temp[16]; // Buffer for the string
    snprintf(temp, sizeof(temp), "Ch: %d", channel_selected);
    int width = u8g2.getStrWidth(temp);
    // Right-align: set cursor to screen width minus width (adjust Y as needed)
    u8g2.setCursor(128 - width, 6); // Assuming same Y position
    u8g2.print("Ch: ");
    u8g2.print(channel_selected);

    // Draw the ADSR envelope lines
    // Attack: from min to max
    u8g2.drawLine(GRAPH_X, y_max, x_attack, y_min);
    // Decay: from max to sustain
    u8g2.drawLine(x_attack, y_min, x_decay, y_sustain);
    // Sustain: horizontal line
    u8g2.drawLine(x_decay, y_sustain, x_sustain_end, y_sustain);
    // Release: from sustain to min
    u8g2.drawLine(x_sustain_end, y_sustain, x_release, y_max);

    // Draw the total_time
    u8g2.setCursor(0, 64 - 10);

    int division_factor = 10; // Scales down the time to human perception of when each phase occurs
    const char *spacer = " ";
    char buf[16]; // Buffer for formatted strings
    // u8g2.print("A: ");
    float val_attack = (adsr_attack[ch] * 100.0f) / adsr_attack_max;
    if (val_attack >= 99.99f) {
        sprintf(buf, "%.1f", val_attack);
    } else {
        sprintf(buf, "%.2f", val_attack);
    }
    u8g2.print(buf);
    // u8g2.print(spacer);
    u8g2.print(">");
    // u8g2.print("D: ");
    float val_decay = (adsr_decay[ch] * 100.0f) / adsr_decay_max;
    if (val_decay >= 99.99f) {
        sprintf(buf, "%.1f", val_decay);
    } else {
        sprintf(buf, "%.2f", val_decay);
    }
    u8g2.print(buf);
    // u8g2.print(spacer);
    u8g2.print(">");
    // u8g2.print("S: ");
    int sustain_percent = round((adsr_sustain[ch] * 100.0) / 4095.0);
    u8g2.print(sustain_percent);
    u8g2.print("%");
    u8g2.print(">");
    // u8g2.print("R: ");
    float val_release = (adsr_release[ch] * 100.0f) / adsr_release_max;
    if (val_release >= 99.99f) {
        sprintf(buf, "%.1f", val_release);
    } else {
        sprintf(buf, "%.2f", val_release);
    }
    u8g2.print(buf);
    // u8g2.print(spacer);
    //u8g2.print("ms");
    
    oledUpdateNeeded = true; // Ensure we update
}

void oledUpdate()
{
    unsigned long currentMillis = millis();

    // Check if it's time to update the counter
    if (currentMillis - previousMillis >= refreshInterval)
    {
        previousMillis = currentMillis;

        // if currentState changes, clear the whole display
        // if (currentState != lastDisplayedState)
        {
            clearArea(0, 64, 128, 65, 0);

            // lastDisplayedState = currentState; // Update the last displayed state
            oledUpdateNeeded = true; // Set the flag to update the display
        }

        // Only update normal display if no popup is active
        // if (!popupActive)
        {
            if (currentState == ADSR_SCREEN)
            {
                // Update the parameters state
                displayParametersState();
            }
            else //if (currentState == MENU_SCREEN)
            {
                // Update the Menu (values) state
                displayMenuState();
            }
        }

        // Send all drawing commands to the display
        if (oledUpdateNeeded)
        {
            //   if (!spiBusy)
            {
                //       spiBusy = true;

                // Draw every pixel to measure active area
                // u8g2.clearBuffer();
                // u8g2.drawBox(0, 0, 128, 64);

                u8g2.sendBuffer();
                //       spiBusy = false;
                oledUpdateNeeded = false; // Reset the flag after updating
            }
            // If SPI busy, leave oledUpdateNeeded = true and try again next cycle
        }
    }
}
/*
void handleEncoderSwitch()
{

    else if (currentState == SCALE_MENU)
    {
        currentScale = highlightedValue; // Set the rhythm generator type
        bufferedSerialPrint("Entering Scale state - currentScale: ");
        bufferedSerialPrint(currentScale);
        bufferedSerialPrintln();

        if (currentScale == 0)
        {
            // No quantisation
            bufferedSerialPrint("No quantisation selected");
            bufferedSerialPrintln();
        }
        else if (currentScale == 1)
        {
            // Chromatic scale
            bufferedSerialPrint("Chromatic scale selected");
            bufferedSerialPrintln();
        }
        else if (currentScale == 2)
        {
            // Major scale
            bufferedSerialPrint("Major scale selected");
            bufferedSerialPrintln();
        }
        else
        {
            // Other scales can be added here
            bufferedSerialPrint("Other scale selected");
            bufferedSerialPrintln();
        }

        highlightedValue = 0;
        lastPositionValue = (step / 4);

        currentState = QUANTISER_MENU;
    }
    else if (currentState == KEY_MENU)
    {
        Key = highlightedValue; // Set the rhythm generator type
        bufferedSerialPrint("Entering Key state - Key: ");
        bufferedSerialPrint(Key);
        bufferedSerialPrintln();

        highlightedValue = 0;
        lastPositionValue = (step / 4);

        currentState = QUANTISER_MENU;
    }
    else if (currentState == MEMORY_MENU)
    {
        saveOrLoadState();
    }
    else if (currentState == COPY_MENU)
    {
        switch (highlightedValue)
        {
        case 0: // Toggle Rhythm Copy
            copyRhythm = !copyRhythm;
            break;
        case 1: // Toggle CV Copy
            copyCvOut = !copyCvOut;
            break;
        case 2: // Toggle Gate Copy
            copyGateLength = !copyGateLength;
            break;
        case 3: // Toggle Glide Copy
            copyGlide = !copyGlide;
            break;
        case 4: // Toggle Probability Copy
            copyProbability = !copyProbability;
            break;
        case 5: // Toggle Roll Copy
            copyRoll = !copyRoll;
            break;
        }
    }

    oledUpdateNeeded = true;
    // printCvChannelNames();
}
  */

void enterMenu()
{
    // Prevent rapid toggling between states
    static unsigned long lastStateChangeTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastStateChangeTime < 100)
    { // 100ms debounce
        return;
    }
    lastStateChangeTime = currentTime;

    // Moving from PARAMETERS to a selection menu
    currentState = MENU_SCREEN; // Update the state

    // Initialise highlightedValue to match the current selection
    //if (newState == MEMORY_MENU && currentSaveSlot != 0)
    {
 //       highlightedValue = currentSaveSlot; // Current save slot is now directly the menu index
    }
  //  else
    {
        highlightedValue = 0; // Reset to the first item in the list
    }

    // Flag that we need to reset the encoder tracking on next oledUpdate
    oledUpdateNeeded = true;
}
   
void displayMenuState()
{
    // maxValue switch
   //switch (currentState)
   // {
    //case VALUES_CVCHANNEL:
        maxValue = numMenuItems; // Number of menu items
     //   break;
    //}

    // Get the encoder position and step
    int position, step;
    getEncoderPosition(&position, &step);

    if ((step / 4) != lastPositionValue)
    {
        // Calculate new position relative to the current state
        int newPos = highlightedValue + ((step / 4) - lastPositionValue);

        // Ensure position wraps correctly
        newPos = ((newPos % maxValue) + maxValue) % maxValue;

        // Update highlighted value
        highlightedValue = newPos;

        // Store current encoder position
        lastPositionValue = (step / 4);

        // Clear display and request update
        clearArea(0, 64, 128, 64, 0);
        oledUpdateNeeded = true;
    }

    // Improved display window calculation to ensure the highlighted value is always visible
    int displayStartIndex;
    int displayEndIndex;

    // Make sure highlighted value is always visible in the window
    if (highlightedValue < 2)
    {
        // At the top of the list
        displayStartIndex = 0;
    }
    else if (highlightedValue >= maxValue - 3)
    {
        // At the bottom of the list
        displayStartIndex = max(0, maxValue - 5);
    }
    else
    {
        // Middle of the list - keep highlighted value in the middle
        displayStartIndex = highlightedValue - 2;
    }

    displayEndIndex = min(maxValue, displayStartIndex + 5);

    // Draw the title
    clearArea(0, 9, 128, 9, 0); // Clear title area before drawing new title
    u8g2.setFont(smallFont);
    u8g2.setCursor(3, 7);
    if (currentState == MENU_SCREEN)
    {
        u8g2.print("Menu");
    }
    
    // Draw the horizontal line below the title
    if (highlightedValue == 0)
    {
        u8g2.setDrawColor(0); // Black
    }
    else
    {
        u8g2.setDrawColor(1); // White
    }
    u8g2.drawLine(3, 9, 117, 9);

    // Draw the list of values
    for (int i = displayStartIndex; i < displayEndIndex; i++)
    {
        u8g2.setCursor(0, 17 + ((i - displayStartIndex) * 10));
        if (i == highlightedValue)
        {
            u8g2.setDrawColor(1); // White
            u8g2.drawBox(0, ((i - displayStartIndex) * 10) + 9, 128, 10);
            u8g2.setDrawColor(0); // Black
        }
        else
        {
            u8g2.setDrawColor(1); // White
        }
        u8g2.setCursor(3, 17 + ((i - displayStartIndex) * 10));
      

        if (currentState == MENU_SCREEN)
        {
            u8g2.print(menuItemNames[i]);
        }
    }
    //if (currentState == VALUES_CVCHANNEL)
    {
        oledUpdateNeeded = true; // Ensure we update the display to display the CV changing
    }

    // Display arrow indicating the highlighted value
    u8g2.setDrawColor(0); // Black
    u8g2.setCursor(116, ((highlightedValue - displayStartIndex) * 10) + 17);
    u8g2.print("<");

    // Display scroll indicators when necessary
    u8g2.setDrawColor(1); // White
    if (displayStartIndex > 0)
    {
        u8g2.setCursor(120, 17);
        u8g2.print("^");
    }
    if (displayEndIndex < maxValue)
    {
        u8g2.setCursor(120, 57);
        u8g2.print("v");
    }
}