#include "SFPlayer.h"

#include <foobar2000.h>

static void* g_sfloader_callback_open(const char* filename) {
	file::ptr* ptr = NULL;
	try {
		abort_callback_dummy _abort;
		pfc::string8 _filename = "";
		if(strstr(filename, "://") == 0)
			_filename = "file://";
		_filename += filename;

		ptr = new file::ptr;
		filesystem::g_open(*ptr, _filename, filesystem::open_mode_read, _abort);

		return (void*)ptr;
	} catch(...) {
		return NULL;
	}
}

static int g_sfloader_callback_read(void* buf, fluid_long_long_t count, void* handle) {
	try {
		abort_callback_dummy _abort;
		file::ptr* ptr = (file::ptr*)handle;
		(*ptr)->read_object(buf, count, _abort);
		return FLUID_OK;
	} catch(...) {
		return FLUID_FAILED;
	}
}

static int g_sfloader_callback_seek(void* handle, fluid_long_long_t offset, int origin) {
	try {
		abort_callback_dummy _abort;
		file::ptr* ptr = (file::ptr*)handle;
		(*ptr)->seek_ex(offset, (file::t_seek_mode)origin, _abort);
		return FLUID_OK;
	} catch(...) {
		return FLUID_FAILED;
	}
}

static int g_sfloader_callback_close(void* handle) {
	try {
		file::ptr* ptr = (file::ptr*)handle;
		delete ptr;
		return FLUID_OK;
	} catch(...) {
		return FLUID_FAILED;
	}
}

static fluid_long_long_t g_sfloader_callback_tell(void* handle) {
	try {
		abort_callback_dummy _abort;
		file::ptr* ptr = (file::ptr*)handle;
		return (*ptr)->get_position(_abort);
	} catch(...) {
		return FLUID_FAILED;
	}
}

static fluid_sfloader_t* g_get_sfloader(fluid_settings_t* settings) {
	fluid_sfloader_t* ret = new_fluid_defsfloader(settings);
	if(ret) {
		if(fluid_sfloader_set_callbacks(ret, g_sfloader_callback_open,
		                                g_sfloader_callback_read, g_sfloader_callback_seek,
		                                g_sfloader_callback_tell, g_sfloader_callback_close) == FLUID_OK)
			return ret;
	}
	return NULL;
}

SFPlayer::SFPlayer()
: MIDIPlayer() {
	_synth[0] = 0;
	_synth[1] = 0;
	_synth[2] = 0;
	uInterpolationMethod = FLUID_INTERP_DEFAULT;
	bDynamicLoading = true;
	bEffects = true;
	uVoices = 256;

	for(unsigned int i = 0; i < 3; ++i) {
		_settings[i] = new_fluid_settings();

		fluid_settings_setnum(_settings[i], "synth.gain", 0.2);
		fluid_settings_setnum(_settings[i], "synth.sample-rate", 44100);
		fluid_settings_setint(_settings[i], "synth.midi-channels", 16);
		fluid_settings_setint(_settings[i], "synth.dynamic-sample-loading", bDynamicLoading ? 1 : 0);
		fluid_settings_setint(_settings[i], "synth.device-id", 0x10 + i);
		fluid_settings_setint(_settings[i], "synth.reverb.active", bEffects ? 1 : 0);
		fluid_settings_setint(_settings[i], "synth.chorus.active", bEffects ? 1 : 0);
		fluid_settings_setint(_settings[i], "synth.polyphony", uVoices);
	}
}

SFPlayer::~SFPlayer() {
	for(unsigned int i = 0; i < 3; ++i) {
		if(_synth[i]) delete_fluid_synth(_synth[i]);
		if(_settings[i]) delete_fluid_settings(_settings[i]);
	}
}

void SFPlayer::setInterpolationMethod(unsigned method) {
	uInterpolationMethod = method;
	for(unsigned int i = 0; i < 3; ++i)
		if(_synth[i]) fluid_synth_set_interp_method(_synth[i], -1, method);
}

void SFPlayer::setDynamicLoading(bool enabled) {
	if(bDynamicLoading != enabled)
		shutdown();
	bDynamicLoading = enabled;
}

void SFPlayer::setEffects(bool enabled) {
	if(bEffects != enabled) {
		for(unsigned i = 0; i < 3; ++i) {
			fluid_settings_setint(_settings[i], "synth.reverb.active", enabled ? 1 : 0);
			fluid_settings_setint(_settings[i], "synth.chorus.active", enabled ? 1 : 0);
		}
	}
	bEffects = enabled;
}

void SFPlayer::setVoiceCount(unsigned int voices) {
	if(uVoices != voices) {
		for(unsigned i = 0; i < 3; ++i) {
			fluid_settings_setint(_settings[i], "synth.polyphony", voices);
		}
	}
	uVoices = voices;
}

void SFPlayer::send_event(uint32_t b) {
	int param2 = (b >> 16) & 0xFF;
	int param1 = (b >> 8) & 0xFF;
	int cmd = b & 0xF0;
	int chan = b & 0x0F;
	int port = (b >> 24) & 0x7F;
	fluid_synth_t* _synth = this->_synth[0];

	if(port && port < 3)
		_synth = this->_synth[port];

	switch(cmd) {
		case 0x80:
			fluid_synth_noteoff(_synth, chan, param1);
			break;
		case 0x90:
			fluid_synth_noteon(_synth, chan, param1, param2);
			break;
		case 0xA0:
			break;
		case 0xB0:
			fluid_synth_cc(_synth, chan, param1, param2);
			break;
		case 0xC0:
			fluid_synth_program_change(_synth, chan, param1);
			break;
		case 0xD0:
			fluid_synth_channel_pressure(_synth, chan, param1);
			break;
		case 0xE0:
			fluid_synth_pitch_bend(_synth, chan, (param2 << 7) | param1);
			break;
	}
}

void SFPlayer::send_sysex(const uint8_t* event, uint32_t size, size_t port) {
	if(port >= 3)
		port = 0;
	if(event && size > 2 && event[0] == 0xF0 && event[size - 1] == 0xF7) {
		++event;
		size -= 2;
		fluid_synth_sysex(_synth[0], (const char*)event, size, NULL, NULL, NULL, 0);
		fluid_synth_sysex(_synth[1], (const char*)event, size, NULL, NULL, NULL, 0);
		fluid_synth_sysex(_synth[2], (const char*)event, size, NULL, NULL, NULL, 0);
	}
}

void SFPlayer::render(float* out, unsigned long count) {
	unsigned long done = 0;
	memset(out, 0, sizeof(float) * 2 * count);
	while(done < count) {
		float buffer[512 * 2];
		unsigned long todo = count - done;
		unsigned long i;
		if(todo > 512) todo = 512;
		for(unsigned long j = 0; j < 3; ++j) {
			memset(buffer, 0, sizeof(buffer));
			fluid_synth_write_float(_synth[j], todo, buffer, 0, 2, buffer, 1, 2);
			for(i = 0; i < todo; ++i) {
				out[i * 2 + 0] += buffer[i * 2 + 0];
				out[i * 2 + 1] += buffer[i * 2 + 1];
			}
		}
		out += todo * 2;
		done += todo;
	}
}

void SFPlayer::setSoundFont(const char* in) {
	sSoundFontName = in;
	shutdown();
}

void SFPlayer::setFileSoundFont(const char* in) {
	sFileSoundFontName = in;
	shutdown();
}

bool SFPlayer::reset() {
	unsigned int synths_reset = 0;
	for(unsigned int i = 0; i < 3; ++i) {
		if(_synth[i]) {
			fluid_synth_system_reset(_synth[i]);
			sysex_reset(i, 0);
			++synths_reset;
		}
	}
	return synths_reset == 3;
}

void SFPlayer::shutdown() {
	for(unsigned int i = 0; i < 3; ++i) {
		if(_synth[i]) delete_fluid_synth(_synth[i]);
		_synth[i] = 0;
	}
	initialized = false;
}

bool SFPlayer::startup() {
	if(_synth[0] && _synth[1] && _synth[2]) return true;

	for(unsigned int i = 0; i < 3; ++i) {
		fluid_settings_setnum(_settings[i], "synth.sample-rate", uSampleRate);
		fluid_settings_setint(_settings[i], "synth.dynamic-sample-loading", bDynamicLoading ? 1 : 0);

		fluid_sfloader_t* _loader = g_get_sfloader(_settings[i]);
		if(!_loader) {
			_last_error = "Out of memory";
			return false;
		}

		_synth[i] = new_fluid_synth(_settings[i]);
		if(!_synth[i]) {
			_last_error = "Out of memory";
			return false;
		}
		fluid_synth_add_sfloader(_synth[i], _loader);
		fluid_synth_set_interp_method(_synth[i], -1, uInterpolationMethod);
	}
	if(sSoundFontName.length()) {
		std::string ext;
		size_t dot = sSoundFontName.find_last_of('.');
		if(dot != std::string::npos)
			ext.assign(sSoundFontName.begin() + dot + 1, sSoundFontName.end());
		if(!stricmp(ext.c_str(), "sf2") || !stricmp(ext.c_str(), "sf3")) {
			for(unsigned int i = 0; i < 3; ++i) {
				if(FLUID_FAILED == fluid_synth_sfload(_synth[i], sSoundFontName.c_str(), 1)) {
					shutdown();
					_last_error = "Failed to load SoundFont bank: ";
					_last_error += sSoundFontName;
					return false;
				}
			}
		} else if(!stricmp_utf8(ext.c_str(), "sflist")) {
			FILE* fl = _tfopen(pfc::stringcvt::string_os_from_utf8(sSoundFontName.c_str()), _T("r, ccs=UTF-8"));
			if(fl) {
#ifdef UNICODE
				std::wstring path, temp;
#else
				std::string path, temp;
#endif
				TCHAR name[32768];
				size_t slash = sSoundFontName.find_last_of('\\');
				if(slash != std::string::npos)
					path.assign(sSoundFontName.begin(), sSoundFontName.begin() + slash + 1);
				while(!feof(fl)) {
					if(!_fgetts(name, 32767, fl)) break;
					name[32767] = 0;
					TCHAR* cr = _tcschr(name, '\n');
					if(cr) *cr = 0;
					cr = _tcschr(name, '\r');
					if(cr) *cr = 0;
					if((isalpha(name[0]) && name[1] == ':') || name[0] == '\\') {
						temp = name;
					} else {
						temp = path;
						temp += name;
					}
					for(unsigned int i = 0; i < 3; ++i) {
						if(FLUID_FAILED == fluid_synth_sfload(_synth[i], pfc::stringcvt::string_utf8_from_os(temp.c_str()), 1)) {
							fclose(fl);
							shutdown();
							_last_error = "Failed to load SoundFont bank: ";
							_last_error += pfc::stringcvt::string_utf8_from_os(cr);
							return false;
						}
					}
				}
				fclose(fl);
			} else {
				_last_error = "Failed to open SoundFont list: ";
				_last_error += sSoundFontName;
				return false;
			}
		}
	}

	if(sFileSoundFontName.length()) {
		for(unsigned int i = 0; i < 3; ++i) {
			if(FLUID_FAILED == fluid_synth_sfload(_synth[i], sFileSoundFontName.c_str(), 1)) {
				shutdown();
				_last_error = "Failed to load SoundFont bank: ";
				_last_error += sFileSoundFontName;
				return false;
			}
		}
	}

	initialized = true;

	setFilterMode(mode, reverb_chorus_disabled);

	_last_error = "";

	return true;
}

bool SFPlayer::get_last_error(std::string& p_out) {
	if(_last_error.length()) {
		p_out = _last_error;
		return true;
	}

	return false;
}
