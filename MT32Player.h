#ifndef __MT32Player_h__
#define __MT32Player_h__

#include "MIDIPlayer.h"

#include <mt32emu.h>

namespace foobar2000_io {
	class abort_callback;
};

class MT32Player : public MIDIPlayer {
	public:
	// zero variables
	MT32Player(bool gm = false, unsigned gm_set = 0);

	// close, unload
	virtual ~MT32Player();

	// configuration
	void setBasePath(const char *in);
	void setAbortCallback(foobar2000_io::abort_callback *in);

	bool isConfigValid();

	static int getSampleRate();

	protected:
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t *event, uint32_t size, size_t port);
	virtual void render(audio_sample *out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	MT32Emu::Synth *_synth;
	pfc::string8 sBasePath;
	foobar2000_io::abort_callback *_abort;

	MT32Emu::File *controlRomFile, *pcmRomFile;
	const MT32Emu::ROMImage *controlRom, *pcmRom;

	bool bGM;
	unsigned uGMSet;

	void _reset();

	MT32Emu::File *openFile(const char *filename);
};

#endif
