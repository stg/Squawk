#ifndef _SQUAWKSD_H_
#define _SQUAWKSD_H_
#include <Squawk.h>
#include <SD.h>

class SquawkSynthSD : public SquawkSynth {
  private:
  	File f;
	public:
	  inline void play() { Squawk.play(); };
		void play(File file);
		void convert(File in, File out);
};

extern SquawkSynthSD SquawkSD;

#endif