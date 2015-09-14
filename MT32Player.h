#ifndef __MT32Player_h__
#define __MT32Player_h__

#include "MIDIPlayer.h"

#include <mt32emu.h>

namespace foobar2000_io
{
	class abort_callback;
};

class MT32Player : public MIDIPlayer
{
public:
	// zero variables
	MT32Player( bool gm = false, unsigned gm_set = 0 );

	// close, unload
	virtual ~MT32Player();

	// configuration
	void setBasePath( const char * in );
	void setAbortCallback( abort_callback * in );

	static int getSampleRate();

protected:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

private:
	MT32Emu::Synth   * _synth;
	pfc::string8       sBasePath;
	abort_callback   * _abort;

	MT32Emu::File    * controlRomFile, * pcmRomFile;
	const MT32Emu::ROMImage * controlRom, * pcmRom;

	bool               bGM;
	unsigned           uGMSet;

	void               reset();

	MT32Emu::File * openFile( const char *filename );
};

#endif
