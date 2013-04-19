/**
SquawkSD16

Author: Xun Yang

Similar to SquawkSD, while this one replaces 
the standard SD to Fat16. It leads to a reduced
compile size(5k bytes). It's especially suitable
for complicated projects limited in space.

Side effect is, Fat16 is more limited comparing
to standard SD in what it can do. There's also 
a few differences in how you initialize the 
sd card and file. See the example 
"SquawkSD16_player.ino" for the whole workflow.

Website for Fat16 https://code.google.com/p/fat16lib/

Note: convert() is not available in this version.
*/
#ifndef _SQUAWKSD_H_
#define _SQUAWKSD_H_
#include <Squawk.h>
#include <Fat16.h>

class SquawkSynthSD16 : public SquawkSynth {
  private:
  	Fat16 f;
	public:
	  inline void play() { Squawk.play(); };
		void play(Fat16 file);
};

extern SquawkSynthSD16 SquawkSD;

#endif