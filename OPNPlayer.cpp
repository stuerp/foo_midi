#include <foobar2000.h>

// Remove Windows versions and use std namespace versions
#undef min
#undef max

#include "OPNPlayer.h"

#include "../../libOPNMIDI/include/opnmidi.h"

#include "Tomsoft.wopn.h"
#include "fmmidi.wopn.h"
#include "gems-fmlib-gmize.wopn.h"
#include "gs-by-papiezak-and-sneakernets.wopn.h"
#include "xg.wopn.h"

OPNPlayer::OPNPlayer()
: MIDIPlayer() {
	memset(midiplay, 0, sizeof(midiplay));
}

OPNPlayer::~OPNPlayer() {
	shutdown();
}

void OPNPlayer::send_event(uint32_t b) {
	if(!(b & 0x80000000)) {
		unsigned char event[3];
		event[0] = (unsigned char)b;
		event[1] = (unsigned char)(b >> 8);
		event[2] = (unsigned char)(b >> 16);
		unsigned port = (b >> 24) & 3;
		unsigned channel = b & 0x0F;
		unsigned command = b & 0xF0;
		if(port > 2)
			port = 0;
		switch(command) {
			case 0x80:
				opn2_rt_noteOff(midiplay[port], channel, event[1]);
				break;

			case 0x90:
				opn2_rt_noteOn(midiplay[port], channel, event[1], event[2]);
				break;

			case 0xA0:
				opn2_rt_noteAfterTouch(midiplay[port], channel, event[1], event[2]);
				break;

			case 0xB0:
				opn2_rt_controllerChange(midiplay[port], channel, event[1], event[2]);
				break;

			case 0xC0:
				opn2_rt_patchChange(midiplay[port], channel, event[1]);
				break;

			case 0xD0:
				opn2_rt_channelAfterTouch(midiplay[port], channel, event[1]);
				break;

			case 0xE0:
				opn2_rt_pitchBendML(midiplay[port], channel, event[2], event[1]);
				break;
		}
	} else {
		uint32_t n = b & 0xffffff;
		const uint8_t* data;
		size_t size, port;
		mSysexMap.get_entry(n, data, size, port);
		opn2_rt_systemExclusive(midiplay[0], data, size);
		opn2_rt_systemExclusive(midiplay[1], data, size);
		opn2_rt_systemExclusive(midiplay[2], data, size);
	}
}

void OPNPlayer::render(float* out, unsigned long count) {
	signed short buffer[512];

	while(count) {
		unsigned todo = count;
		if(todo > 256) todo = 256;

		memset(out, 0, todo * sizeof(float) * 2);

		for(unsigned i = 0; i < 3; i++) {
			opn2_generate(midiplay[i], todo * 2, buffer);

			for(unsigned j = 0, k = todo * 2; j < k; j++) {
				out[j] += (float)buffer[j] * (1.0f / 32768.0f);
			}
		}

		out += todo * 2;
		count -= todo;
	}
}

void OPNPlayer::setCore(unsigned core) {
	uEmuCore = core;
}

void OPNPlayer::setBank(unsigned bank) {
	uBankNumber = bank;
}

void OPNPlayer::setChipCount(unsigned count) {
	uChipCount = count;
}

void OPNPlayer::setFullPanning(bool enable) {
	bFullPanning = enable;
}

void OPNPlayer::shutdown() {
	for(unsigned i = 0; i < 3; i++)
		opn2_close(midiplay[i]);
	memset(midiplay, 0, sizeof(midiplay));
}

bool OPNPlayer::startup() {
	if(midiplay[0] && midiplay[1] && midiplay[2]) return true;

	int chips_per_port = uChipCount / 3;
	int chips_round = (uChipCount % 3) != 0;
	int chips_min = uChipCount < 3;

	for(unsigned i = 0; i < 3; i++) {
		OPN2_MIDIPlayer* midiplay = this->midiplay[i] = opn2_init(uSampleRate);
		if(!midiplay) return false;

		const void* _bank;
		size_t _banksize;

		switch(uBankNumber) {
			default:
			case 0:
				_bank = bnk_xg;
				_banksize = sizeof(bnk_xg);
				break;

			case 1:
				_bank = bnk_gs;
				_banksize = sizeof(bnk_gs);
				break;

			case 2:
				_bank = bnk_gems;
				_banksize = sizeof(bnk_gems);
				break;

			case 3:
				_bank = bnk_Tomsoft;
				_banksize = sizeof(bnk_Tomsoft);
				break;

			case 4:
				_bank = bnk_fmmidi;
				_banksize = sizeof(bnk_fmmidi);
				break;
		}

		opn2_openBankData(midiplay, _bank, _banksize);

		opn2_setNumChips(midiplay, chips_per_port + chips_round * (i == 0) + chips_min * (i != 0));
		opn2_setSoftPanEnabled(midiplay, bFullPanning);
		opn2_setDeviceIdentifier(midiplay, i);
		opn2_switchEmulator(midiplay, uEmuCore);
		opn2_reset(midiplay);
	}

	return true;
}
