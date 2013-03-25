#ifndef _SQUAWKSD_H_
#define _SQUAWKSD_H_
#include <Squawk.h>
#include <SD.h>

class SquawkSynthSD : public SquawkSynth {
  private:
  	File f;
	public:
		void play(File file);
};

extern SquawkSynthSD SquawkSD;

#endif