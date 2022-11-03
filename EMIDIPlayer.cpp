#include "EMIDIPlayer.h"

#include "shared.h"

static const uint8_t sysex_gm_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t sysex_gm2_reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t sysex_gs_reset[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t sysex_xg_reset[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static bool is_gs_reset(const unsigned char* data, unsigned long size) {
	if(size != _countof(sysex_gs_reset)) return false;

	if(memcmp(data, sysex_gs_reset, 5) != 0) return false;
	if(memcmp(data + 7, sysex_gs_reset + 7, 2) != 0) return false;
	if(((data[5] + data[6] + 1) & 127) != data[9]) return false;
	if(data[10] != sysex_gs_reset[10]) return false;

	return true;
}

EMIDIPlayer::EMIDIPlayer()
: MIDIPlayer() {
	bInitialized = false;
	synth_mode = mode_gm;
}

EMIDIPlayer::~EMIDIPlayer() {
	shutdown();
}

void EMIDIPlayer::send_event(uint32_t b) {
	dsa::CMIDIMsgInterpreter mi;

	unsigned char event[3];
	event[0] = (unsigned char)b;
	event[1] = (unsigned char)(b >> 8);
	event[2] = (unsigned char)(b >> 16);
	unsigned channel = b & 0x0F;
	unsigned command = b & 0xF0;

	mi.Interpret(event[0]);
	if(event[0] < 0xF8) {
		mi.Interpret(event[1]);
		if((b & 0xF0) != 0xC0 && (b & 0xF0) != 0xD0) mi.Interpret(event[2]);
	}

	if(command == 0xB0 && event[1] == 0) {
		if(synth_mode == mode_xg) {
			if(event[2] == 127)
				drum_channels[channel] = 1;
			else
				drum_channels[channel] = 0;
		} else if(synth_mode == mode_gm2) {
			if(event[2] == 120)
				drum_channels[channel] = 1;
			else if(event[2] == 121)
				drum_channels[channel] = 0;
		}
	} else if(command == 0xC0) {
		set_drum_channel(channel, drum_channels[channel]);
	}

	while(mi.GetMsgCount()) {
		const dsa::CMIDIMsg& msg = mi.GetMsg();
		mModule[(msg.m_ch * 2) & 7].SendMIDIMsg(msg);
		if(!drum_channels[msg.m_ch]) mModule[(msg.m_ch * 2 + 1) & 7].SendMIDIMsg(msg);
		mi.PopMsg();
	}
}

void EMIDIPlayer::send_sysex(const uint8_t* event, uint32_t size, size_t port) {
	dsa::CMIDIMsgInterpreter mi;

	for(uint32_t n = 0; n < size; ++n) mi.Interpret(event[n]);

	if((size == _countof(sysex_gm_reset) && !memcmp(event, &sysex_gm_reset[0], _countof(sysex_gm_reset))) ||
	   (size == _countof(sysex_gm2_reset) && !memcmp(event, &sysex_gm2_reset[0], _countof(sysex_gm2_reset))) ||
	   is_gs_reset(event, size) ||
	   (size == _countof(sysex_xg_reset) && !memcmp(event, &sysex_xg_reset[0], _countof(sysex_xg_reset)))) {
		reset_drum_channels();
		synth_mode = (size == _countof(sysex_xg_reset)) ? mode_xg :
		                                                  (size == _countof(sysex_gs_reset)) ? mode_gs :
		                                                                                       (event[4] == 0x01) ? mode_gm :
		                                                                                                            mode_gm2;
	} else if(synth_mode == mode_gs && size == 11 &&
	          event[0] == 0xF0 && event[1] == 0x41 && event[3] == 0x42 &&
	          event[4] == 0x12 && event[5] == 0x40 && (event[6] & 0xF0) == 0x10 &&
	          event[10] == 0xF7) {
		if(event[7] == 2) {
			// GS MIDI channel to part assign
			gs_part_to_ch[event[6] & 15] = event[8];
		} else if(event[7] == 0x15) {
			// GS part to rhythm allocation
			unsigned int drum_channel = gs_part_to_ch[event[6] & 15];
			if(drum_channel < 16) {
				drum_channels[drum_channel] = event[8];
			}
		}
	}

	while(mi.GetMsgCount()) {
		const dsa::CMIDIMsg& msg = mi.GetMsg();
		mModule[(msg.m_ch * 2) & 7].SendMIDIMsg(msg);
		if(!drum_channels[msg.m_ch]) mModule[(msg.m_ch * 2 + 1) & 7].SendMIDIMsg(msg);
		mi.PopMsg();
	}
}

void EMIDIPlayer::render(audio_sample* out, unsigned long count) {
	INT32 b[256 * sizeof(audio_sample)];

	while(count) {
		unsigned todo = 256;

		if(todo > count)
			todo = (unsigned)count;

		memset(b, 0, (todo * 2) * sizeof(audio_sample));

		for(unsigned i = 0; i < 8; ++i) {
			for(unsigned j = 0; j < todo; ++j) {
				INT32 c[2];
				mModule[i].Render(c);
				b[j * 2] += c[0];
				b[j * 2 + 1] += c[1];
			}
		}

		audio_math::convert_from_int32((const t_int32*)b, (todo * 2), out, 1 << 16);

		out += (todo * 2);
		count -= todo;
	}
}

bool EMIDIPlayer::startup() {
	if(bInitialized) return true;

	for(unsigned i = 0; i < 8; ++i) {
		if(i & 1)
			mModule[i].AttachDevice(new dsa::CSccDevice(uSampleRate, 2));
		else
			mModule[i].AttachDevice(new dsa::COpllDevice(uSampleRate, 2));
		mModule[i].Reset();
	}

	reset_drum_channels();

	bInitialized = true;

	return true;
}

void EMIDIPlayer::shutdown() {
	if(bInitialized)
		for(unsigned i = 0; i < 8; ++i) {
			delete mModule[i].DetachDevice();
		}

	bInitialized = false;
}

void EMIDIPlayer::reset_drum_channels() {
	static const uint8_t part_to_ch[16] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15 };

	memset(drum_channels, 0, sizeof(drum_channels));
	drum_channels[9] = 1;

	memcpy(gs_part_to_ch, part_to_ch, sizeof(gs_part_to_ch));

	for(unsigned i = 0; i < 16; ++i) {
		set_drum_channel(i, drum_channels[i]);
	}
}

void EMIDIPlayer::set_drum_channel(int channel, int enable) {
	for(unsigned i = 0; i < 8; ++i) {
		mModule[i].SetDrumChannel(channel, enable);
	}
}
