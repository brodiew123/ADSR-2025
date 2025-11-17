This is a quad ADSR module for Eurorack made using a Raspberry Pi Pico, just the standard version.

This is my first upload so I've kept it simple, so let me know if you'd like more information about this. 

With an I2C OLED screen and 4 encoders, you can navigate between the 4 simultaneous ADSR channels to set the settings, and each pulse will run independently. Adjustment with the encoders is done logarithmically, so we have fine control at the lower end of the range, and coarser control at the higher end. 

This is setup for basic cheap encoders, and a 128x64 standard I2C OLED screen.

I used Peter Zimon's library for Pico as the core: https://github.com/peterzimon/pico-lib/tree/main?tab=readme-ov-file
And then used the example here and converted that to work on Pico: https://github.com/mo-thunderz/adsr?tab=readme-ov-file
Then the rest of the structure is coded by me. 

One major thing I did fix was the decending Release DAC output. This was being calculated in a way that seemed to be truncated and stepped, so I calculated it in float instead for a really smooth line.

I've left a lot of commented code because I'm yet to add save and load with many save slots, and load on start up. I'll clean it up once I have all the functionality included.

If you use this, let me know, I'd love to hear from you.
