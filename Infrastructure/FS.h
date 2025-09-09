
/** $VER: FS.h (2025.07.26) P. Stuer - FluidSynth bindings **/

#pragma once

#include "Resource.h"

#include <fluidsynth.h>

#include <pfc/string-conv-lite.h>

namespace FluidSynth
{

#pragma region FluidSynth API

typedef void (WINAPIV * _fluid_version)(int * major, int * minor, int * micro);

typedef int (WINAPIV * _fluid_audio_driver_register)(const char ** drivers);

typedef fluid_settings_t * (WINAPIV * _new_fluid_settings)();

typedef int (WINAPIV * _fluid_settings_setnum)(fluid_settings_t * settings, const char * name, double val);
typedef int (WINAPIV * _fluid_settings_setint)(fluid_settings_t * settings, const char * name, int val);
typedef int (WINAPIV * _fluid_settings_setstr)(fluid_settings_t * settings, const char * name, const char * val);

typedef int (WINAPIV * _fluid_settings_get_type)(fluid_settings_t * settings, const char * name);

typedef int (WINAPIV * _fluid_settings_getnum_range)(fluid_settings_t * settings, const char * name, double * min, double * max);
typedef int (WINAPIV * _fluid_settings_getint_range)(fluid_settings_t * settings, const char * name, int * min, int * max);
typedef int (WINAPIV * _fluid_settings_dupstr)(fluid_settings_t * settings, const char * name, char ** val);

typedef int (WINAPIV * _fluid_settings_getnum)(fluid_settings_t * settings, const char * name, double * val);
typedef int (WINAPIV * _fluid_settings_getint)(fluid_settings_t * settings, const char * name, int * val);

typedef int (WINAPIV * _fluid_settings_foreach)(fluid_settings_t * settings, void * data, fluid_settings_foreach_t func);

typedef void (WINAPIV * _fluid_free)(void * data);
typedef void (WINAPIV * _delete_fluid_settings)(fluid_settings_t * settings);

typedef int (WINAPIV * _fluid_is_soundfont)(const char * filePath);

typedef fluid_synth_t * (WINAPIV * _new_fluid_synth)(fluid_settings_t * settings);
typedef void (WINAPIV * _fluid_synth_add_sfloader)(fluid_synth_t * synth, fluid_sfloader_t * loader);
typedef int (WINAPIV * _fluid_synth_system_reset)(fluid_synth_t *synth);
typedef void (WINAPIV * _delete_fluid_synth)(fluid_synth_t * synth);

typedef fluid_sfloader_t * (WINAPIV * _new_fluid_defsfloader)(fluid_settings_t * settings);
typedef int (WINAPIV * _fluid_sfloader_set_callbacks)(fluid_sfloader_t * loader, fluid_sfloader_callback_open_t open, fluid_sfloader_callback_read_t read, fluid_sfloader_callback_seek_t seek, fluid_sfloader_callback_tell_t tell, fluid_sfloader_callback_close_t close);

typedef int (WINAPIV * _fluid_synth_set_interp_method)(fluid_synth_t * synth, int chan, int interp_method);

typedef void (WINAPIV * _fluid_synth_add_sfloader)(fluid_synth_t * synth, fluid_sfloader_t * loader);
typedef int (WINAPIV * _fluid_synth_sfload)(fluid_synth_t * synth, const char * filePath, int reset_presets);
typedef int (WINAPIV * _fluid_synth_set_bank_offset)(fluid_synth_t * synth, int sfont_id, int offset);

typedef const char * (WINAPIV * _fluid_sfont_get_name)(fluid_sfont_t * sfont);
typedef const char * (WINAPIV * _fluid_preset_get_name)(fluid_preset_t * preset);

typedef void (WINAPIV * _fluid_sfont_iteration_start)(fluid_sfont_t * sfont);
typedef fluid_preset_t *(WINAPIV * _fluid_sfont_iteration_next)(fluid_sfont_t * sfont);

typedef fluid_sfont_t * (WINAPIV * _fluid_synth_get_sfont_by_id)(fluid_synth_t * synth, int id);
typedef fluid_preset_t * (WINAPIV * _fluid_sfont_get_preset)(fluid_sfont_t * sfont, int bankNumber, int programNumber);

typedef int (WINAPIV * _fluid_synth_noteon)(fluid_synth_t * synth, int chan, int key, int vel);
typedef int (WINAPIV * _fluid_synth_noteoff)(fluid_synth_t * synth, int chan, int key);
typedef int (WINAPIV * _fluid_synth_key_pressure)(fluid_synth_t * synth, int chan, int key, int val);
typedef int (WINAPIV * _fluid_synth_cc)(fluid_synth_t * synth, int chan, int ctrl, int val);
typedef int (WINAPIV * _fluid_synth_get_cc)(fluid_synth_t * synth, int chan, int ctrl, int *pval);
typedef int (WINAPIV * _fluid_synth_sysex)(fluid_synth_t * synth, const char * data, int len, char * response, int * response_len, int * handled, int dryrun);

typedef int (WINAPIV * _fluid_synth_pitch_bend)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_get_pitch_bend)(fluid_synth_t * synth, int chan, int *ppitch_bend);
typedef int (WINAPIV * _fluid_synth_pitch_wheel_sens)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_get_pitch_wheel_sens)(fluid_synth_t * synth, int chan, int *pval);
typedef int (WINAPIV * _fluid_synth_program_change)(fluid_synth_t * synth, int chan, int program);
typedef int (WINAPIV * _fluid_synth_channel_pressure)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_bank_select)(fluid_synth_t * synth, int chan, int bank);

typedef int (WINAPIV * _fluid_synth_set_reverb_group_roomsize)(fluid_synth_t * synth, int fxGroup, double roomSize);
typedef int (WINAPIV * _fluid_synth_set_reverb_group_damp)(fluid_synth_t * synth, int fxGroup, double damping);
typedef int (WINAPIV * _fluid_synth_set_reverb_group_width)(fluid_synth_t * synth, int fxGroup, double width);
typedef int (WINAPIV * _fluid_synth_set_reverb_group_level)(fluid_synth_t * synth, int fxGroup, double level);
typedef int (WINAPIV * _fluid_synth_set_reverb_on)(fluid_synth_t * synth, int on);

typedef int (WINAPIV * _fluid_synth_set_chorus_nr)(fluid_synth_t * 	synth, int nr);
typedef int (WINAPIV * _fluid_synth_set_chorus_level)(fluid_synth_t * synth, double level);
typedef int (WINAPIV * _fluid_synth_set_chorus_speed)(fluid_synth_t * synth, double speed);
typedef int (WINAPIV * _fluid_synth_set_chorus_depth)(fluid_synth_t * synth, double depth_ms);
typedef int (WINAPIV * _fluid_synth_set_chorus_type)(fluid_synth_t * synth,int type);
typedef int (WINAPIV * _fluid_synth_set_chorus_on)(fluid_synth_t * synth, int on);

typedef int (WINAPIV * _fluid_synth_write_float)(fluid_synth_t * synth, int len, void * lout, int loff, int lincr, void * rout, int roff, int rincr);

typedef int (WINAPIV * _fluid_synth_get_active_voice_count)(fluid_synth_t * synth);

typedef fluid_settings_t * (WINAPIV * _fluid_synth_get_settings)(fluid_synth_t * synth);

typedef fluid_log_function_t (WINAPIV * _fluid_set_log_function)(int level, fluid_log_function_t fun, void * data);

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#define InitializeFunction(type, address) { address = (##_##type) ::GetProcAddress(_hModule, #type); if (*address == nullptr) throw component::runtime_error((std::string("Failed to link FluidSynth function: ") + TOSTRING(type)).c_str()); }

#pragma endregion

/// <summary>
/// Implements a FluidSynth driver.
/// </summary>
class API
{
public:
    API() : GetVersion(), _hModule()
    {
    }

    ~API()
    {
        if (_hModule)
        {
            ::FreeLibrary(_hModule);
            _hModule = 0;
        }
    }

    bool IsInitialized() const noexcept { return (_hModule != 0); }

    void Initialize(const WCHAR * basePath);

    static bool Exists() noexcept;

public:
    _fluid_version GetVersion;

    _fluid_audio_driver_register RegisterDriver;

    _new_fluid_settings CreateSettings;

    _fluid_settings_setnum SetNumericSetting;
    _fluid_settings_setint SetIntegerSetting;
    _fluid_settings_setstr SetStringSetting;

    _fluid_settings_get_type GetSettingType;

    _fluid_settings_getnum GetNumericSetting;
    _fluid_settings_getint GetIntegerSetting;
    _fluid_settings_dupstr GetStringSetting;

    _fluid_settings_getnum_range GetNumericSettingRange;
    _fluid_settings_getint_range GetIntegerSettingRange;

    _fluid_settings_foreach ForEachSetting;

    _fluid_free Free;
    _delete_fluid_settings DeleteSettings;

    _fluid_is_soundfont IsSoundfont;
    
    _new_fluid_synth CreateSynthesizer;
    _fluid_synth_add_sfloader AddSoundfontLoader;
    _fluid_synth_system_reset ResetSynthesizer;
    _delete_fluid_synth DeleteSynthesizer;

    _new_fluid_defsfloader CreateSoundfontLoader;
    _fluid_sfloader_set_callbacks SetSoundfontLoaderCallbacks;
    _fluid_synth_sfload LoadSoundfont;
    _fluid_synth_set_bank_offset SetSoundfontBankOffset;

    _fluid_sfont_get_name GetSoundfontName;
    _fluid_preset_get_name GetPresetName;

    _fluid_synth_get_sfont_by_id GetSoundfont;
    _fluid_sfont_get_preset GetPreset;

    _fluid_synth_set_interp_method SetInterpolationMethod;

    _fluid_synth_noteon NoteOn;
    _fluid_synth_noteoff NoteOff;
    _fluid_synth_key_pressure KeyPressure;
    _fluid_synth_cc ControlChange;
    _fluid_synth_get_cc GetControlChange;
    _fluid_synth_sysex SysEx;

    _fluid_synth_pitch_bend PitchBend;
    _fluid_synth_get_pitch_bend GetPitchBend;
    _fluid_synth_pitch_wheel_sens PitchWheelSensitivity;
    _fluid_synth_get_pitch_wheel_sens GetPitchWheelSensitivity;
    _fluid_synth_program_change ProgramChange;
    _fluid_synth_channel_pressure ChannelPressure;
    _fluid_synth_bank_select BankSelect;

    _fluid_synth_set_reverb_group_roomsize SetReverbRoomSize;
    _fluid_synth_set_reverb_group_damp SetReverbDamp;
    _fluid_synth_set_reverb_group_width SetReverbWidth;
    _fluid_synth_set_reverb_group_level SetReverbLevel;
    _fluid_synth_set_reverb_on SetReverb;

    _fluid_synth_set_chorus_nr SetChorusVoiceCount;
    _fluid_synth_set_chorus_level SetChorusLevel;
    _fluid_synth_set_chorus_speed SetChorusSpeed;
    _fluid_synth_set_chorus_depth SetChorusDepth;
    _fluid_synth_set_chorus_type SetChorusType;
    _fluid_synth_set_chorus_on SetChorus;

    _fluid_synth_write_float WriteFloat;

    _fluid_synth_get_active_voice_count GetActiveVoiceCount;

    _fluid_synth_get_settings GetSettings;

    _fluid_set_log_function SetLogFunction;

private:
    HMODULE _hModule;

    static inline const char * LibraryName = "libfluidsynth-3.dll";
};

class Host
{
public:
    bool LoadConfig(FluidSynth::API & api, const fs::path & filePath, fluid_settings_t * Settings) noexcept;

private:
};

}
