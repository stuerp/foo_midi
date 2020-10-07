#include <foobar2000.h>

// Remove Windows versions and use std namespace versions
#undef min
#undef max

#include "ADLPlayer.h"

#include "../../libADLMIDI/include/adlmidi.h"

ADLPlayer::ADLPlayer(): MIDIPlayer()
{
	memset(midiplay, 0, sizeof(midiplay));
}

ADLPlayer::~ADLPlayer()
{
	shutdown();
}

void ADLPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 3;
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		if (port > 2)
			port = 0;
		switch (command) {
		case 0x80:
			adl_rt_noteOff(midiplay[port], channel, event[1]);
			break;

		case 0x90:
			adl_rt_noteOn(midiplay[port], channel, event[1], event[2]);
			break;

		case 0xA0:
			adl_rt_noteAfterTouch(midiplay[port], channel, event[1], event[2]);
			break;

		case 0xB0:
			adl_rt_controllerChange(midiplay[port], channel, event[1], event[2]);
			break;

		case 0xC0:
			adl_rt_patchChange(midiplay[port], channel, event[1]);
			break;

		case 0xD0:
			adl_rt_channelAfterTouch(midiplay[port], channel, event[1]);
			break;

		case 0xE0:
			adl_rt_pitchBendML(midiplay[port], channel, event[2], event[1]);
			break;
		}
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		adl_rt_systemExclusive(midiplay[0], data, size);
		adl_rt_systemExclusive(midiplay[1], data, size);
		adl_rt_systemExclusive(midiplay[2], data, size);
	}
}

void ADLPlayer::render(float * out, unsigned long count)
{
	signed short buffer[512];

	while ( count )
	{
		unsigned todo = count;
		if ( todo > 256 ) todo = 256;

		memset( out, 0, todo * sizeof( float ) * 2 );

		for (unsigned i = 0; i < 3; i++)
		{
			adl_generate(midiplay[i], todo * 2, buffer);

			for (unsigned j = 0, k = todo * 2; j < k; j++)
			{
				out[j] += (float)buffer[j] * (1.0f / 32768.0f);
			}
		}

		out += todo * 2;
		count -= todo;
	}
}

void ADLPlayer::setCore( unsigned core )
{
	uEmuCore = core;
}

void ADLPlayer::setBank( unsigned bank )
{
	uBankNumber = bank;
}

void ADLPlayer::setChipCount( unsigned count )
{
	uChipCount = count;
}

void ADLPlayer::set4OpCount( unsigned count )
{
	u4OpCount = count;
}

void ADLPlayer::setFullPanning( bool enable )
{
	bFullPanning = enable;
}

void ADLPlayer::shutdown()
{
	for (unsigned i = 0; i < 3; i++)
		adl_close(midiplay[i]);
	memset(midiplay, 0, sizeof(midiplay));
}

bool ADLPlayer::startup()
{
	if ( midiplay[0] && midiplay[1] && midiplay[2] ) return true;

	int chips_per_port = uChipCount / 3;
	int chips_round = (uChipCount % 3) != 0;
	int chips_min = uChipCount < 3;

	for (unsigned i = 0; i < 3; i++)
	{
		ADL_MIDIPlayer * midiplay = this->midiplay[i] = adl_init(uSampleRate);
		if (!midiplay) return false;

		adl_setBank(midiplay, uBankNumber);
		adl_setNumChips(midiplay, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
		adl_setNumFourOpsChn(midiplay, u4OpCount);
		adl_setSoftPanEnabled(midiplay, bFullPanning);
		adl_setDeviceIdentifier(midiplay, i);
		adl_switchEmulator(midiplay, uEmuCore);
		adl_reset(midiplay);
	}

	return true;
}
