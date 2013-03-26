#include <SquawkSD.h>

SquawkSynthSD SquawkSD;

class StreamFile : public SquawkStream {
  private:
    File f;
	public:
		StreamFile(File file = File()) { f = file; }
    virtual uint8_t read() { return f.read(); }
    virtual void seek(size_t offset) { f.seek(offset); }
};

static StreamFile file;

void SquawkSynthSD::play(File melody) {
	SquawkSynth::pause();
	file = StreamFile(melody);
	SquawkSynth::play(&file);
}