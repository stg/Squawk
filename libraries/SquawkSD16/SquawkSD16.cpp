// Author: Xun Yang

#include <SquawkSD16.h>

SquawkSynthSD16 SquawkSD;

class StreamFile : public SquawkStream {
  private:
    Fat16 f;
	public:
		StreamFile(Fat16 file = Fat16()) { f = file; }
    uint8_t read() { return f.read(); }
    void seek(size_t offset) { f.seekSet(offset); }
};

static StreamFile file;

void SquawkSynthSD16::play(Fat16 melody) {
	SquawkSynth::pause();
	file = StreamFile(melody);
	SquawkSynth::play(&file);
}