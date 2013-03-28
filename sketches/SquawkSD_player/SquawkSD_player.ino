/* === SQUAWK MELODY ON SD-CARD PLAYER EXAMPLE === */

#include <Squawk.h>
#include <SD.h>
#include <SquawkSD.h>

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
SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN3)
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN5)
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN11)

// Chip select pin for SD card (it's 4 on the ethernet shield)
const int chipSelect = 4;

// Initialize Squawk
void setup() {
  File melody;
  // The default SD card chip select pin must be output
  pinMode(SS, OUTPUT);
  // Set up Squawk to generate samples at 32kHz.
  // Squawk always steals Timer1 for sample crunching.
  SquawkSD.begin(32000);
  // Initialize the SD card  
  if (SD.begin(chipSelect)) {
    // Open a file on the SD card
    melody = SD.open("melody.sqm");
    // Begin playback of file
    if(melody) SquawkSD.play(melody);
  }
  // If something went wrong, play some noise
  if(!melody) {
    osc[3].vol = 0x0F;
    SquawkSD.play();
  }
}

void loop() {
  // Do whatever you want
}
