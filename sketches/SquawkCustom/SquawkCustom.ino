/* === SQUAWK CUSTOM PLAY-ROUTINE EXAMPLE === */

#include <Squawk.h>

/*
Since Timer0 is used for Arduino functionality, such as delay() it is not
recommended to use a pin that relies on this timer for output, as Squawk
needs to reconfigure the timer for higher-speed, which modifies how these
Arduino internals behave. Uncomment the correct line below, depending on
which Arduino you intend to run the code on.

Note: You CAN use any pin, if you do not rely on Timer0 related things.
      This sketch uses delay() so we need to steer clear of Timer0.

Leonardo
  use SQUAWK_PWM_PIN5 (it's on Timer3)
  
Uno, Due(milanove), Diecimila, Nano, Mini or LilyPad
  use SQUAWK_PWM_PIN11
  or  SQUAWK_PWM_PIN3 (both are on Timer2)
  
Others
  not yet supported, you'll have to try and see what happens ;)
*/

// Configure Squawk for PWM output, and construct suitable ISR.
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN3)
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN5)
//SQUAWK_CONSTRUCT_ISR(SQUAWK_PWM_PIN11)

// Initialize Squawk
void setup() {
  // Set up Squawk to generate samples at 44.1kHz.
  // Squawk always steals Timer1 for sample crunching.
  Squawk.begin(44100);
  // Begin playback.
  Squawk.play();
}

// Plays note using oscillator 2 (triangle with no volume control)
void playNote(byte period, word length, char modifier) {
  // Modifier . makes note length 2/3
  if(modifier == '.') length = (length * 2) / 3;
  // Set up the play frequency, 352800 is [sample_rate]=44100 * [tuning]=8.0
  osc[2].freq = 352800 / period;
  // Delay, silence, delay
  delay(length);
  osc[2].freq = 0;
  delay(length);
}

// A tiny custom play-routine that plays a string containing
// notations for notes, delays and lengths
void customPlay(char *playthis) {
  // Find length of play string
  word length = strlen(playthis);
  // Set the default note time
  word time = 500;
  // Loop through each character in the play string
  for(int n = 0; n < length; n++) {
    // Fetch the character AFTER the current one - it may contain a modifier
    char modifier = playthis[n + 1];
    // Fetch the current character and branch accordingly
    switch(playthis[n]) {
      // Notes
      case 'c': playNote(214, time, modifier); break; // Play a  C
      case 'C': playNote(202, time, modifier); break; // Play a  C#
      case 'd': playNote(190, time, modifier); break; // Play a  D
      case 'D': playNote(180, time, modifier); break; // Play a  D#
      case 'e': playNote(170, time, modifier); break; // Play an F
      case 'f': playNote(160, time, modifier); break; // Play an F
      case 'F': playNote(151, time, modifier); break; // Play an F#
      case 'g': playNote(143, time, modifier); break; // Play a  G
      case 'G': playNote(135, time, modifier); break; // Play a  G#
      case 'a': playNote(127, time, modifier); break; // Play an A
      case 'A': playNote(120, time, modifier); break; // Play an A#
      case 'b': playNote(113, time, modifier); break; // Play a  B
      // Delay
      case '-': playNote(0,   time, modifier); break; // Play a quiet note
      // Note lengths
      case '1': time = 1000; break; // Full note
      case '2': time =  500; break; // Half note
      case '4': time =  250; break; // Quarter note
      case '8': time =   50; break; // Eigth note
    }
  }
}

void loop() {
  // This is what we will play
  char aTinyMelody[] = "8eF-FFga4b.a.g.F.8beee-d2e.1-";
  // Invoke our custom play-routine
  customPlay(aTinyMelody);
}

