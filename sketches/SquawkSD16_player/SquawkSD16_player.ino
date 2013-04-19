/**
SquawkSD16 using Fat16 library

Author: Xun Yang

Fat16 is a bit limited comparing to standard SD, but much slimer.
Website for Fat16 https://code.google.com/p/fat16lib/

When running on Leonardo, comparing to SquawkSD_player, this one goes 
from 21,506 to 16,000 byte. 5k bytes saved!

Note: pin mapping for SD card is defined in SdCard.cpp of Fat16 library.
If you're using a unconventional board, or if the example is not working
when it should, fix the mappings in that file!

*/
#include <Squawk.h>
#include <SquawkSD16.h>
#include <Fat16.h>

/*
Since Timer0 is used for Arduino functionality, such as delay() it is not
recommended to use a pin that relies on this timer for output, as Squawk
needs to reconfigure the timer for higher-speed, which modifies how these
Arduino internals behave. Uncomment the correct line below, depending on
which Arduino you intend to run the code on.

Note: You CAN use any pin, if you do not rely on Timer0 related things.

Leonardo
  use SQUAWK_PWM_PIN5 (it's on Timer3)
  
Uno, Due(milanove), Diecimila, Nano, Mini or LilyPad
  only use SQUAWK_PWM_PIN3 (it's on Timer2)
  because SQUAWK_PWM_PIN11 is used for SD card access
  
Others
  not yet supported, you'll have to try and see what happens ;)
*/

// Configure Squawk for PWM output, and construct suitable ISR.
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN3)
SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN5)
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN11)

Fat16 file;
SdCard card;

// Initialize Squawk
void setup() {
  
  // Set up Squawk to generate samples at 32kHz.
  // Squawk always steals Timer1 for sample crunching.
  SquawkSD.begin(32000);
  // Initialize the SD card  
  if(card.init()) {
    // Open a file on the SD card
    if(file.init(&card)) {
      // Begin playback of file
      if(file.open("Melody.sqm", O_READ)) {
        SquawkSD.play(file);
        return;
      }
    }
  }
  // If something went wrong, play some noise
  osc[3].vol = 0x20;
  SquawkSD.play();
}

void loop() {
  // Do whatever you want
}
