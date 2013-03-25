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

static StreamFile sd;

void SquawkSynthSD::play(File file) {
	SquawkSynth::pause();
	sd = StreamFile(file);
	SquawkSynth::play(&sd);
}