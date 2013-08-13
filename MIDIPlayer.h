#ifndef __MIDIPlayer_h__
#define __MIDIPlayer_h__

#include <foobar2000.h>

#include <midi_container.h>

class MIDIPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	MIDIPlayer();

	// close, unload
	virtual ~MIDIPlayer() {};

	// setup
	void setSampleRate(unsigned rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

protected:
	virtual void send_event(DWORD b) {}
	virtual void render(audio_sample * out, unsigned count) {}

	virtual void shutdown() {};
	virtual bool startup() {return false;}

	unsigned           uSampleRate;
	system_exclusive_table mSysexMap;

private:
	unsigned           uSamplesRemaining;

	unsigned           uLoopMode;

	std::vector<midi_stream_event> mStream;

	UINT               uStreamPosition;
	DWORD              uTimeCurrent;
	DWORD              uTimeEnd;

	UINT               uStreamLoopStart;
	DWORD              uTimeLoopStart;
};

#endif
