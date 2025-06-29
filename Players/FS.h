
/** $VER: FluidSynth.h (2025.06.29) **/

#pragma once

//#include "Player.h"

#include "Resource.h"

#include "Infrastructure\Exception.h"

#include <Encoding.h>

#include <fluidsynth.h>

#include <pfc/string-conv-lite.h>

#include <Shlwapi.h>

#pragma region FluidSynth API

typedef void (WINAPIV * _fluid_version)(int * major, int * minor, int * micro);

typedef fluid_settings_t * (WINAPIV * _new_fluid_settings)();

typedef int (WINAPIV * _fluid_settings_setnum)(fluid_settings_t * settings, const char * name, double val);
typedef int (WINAPIV * _fluid_settings_setint)(fluid_settings_t * settings, const char * name, int val);
typedef int (WINAPIV * _fluid_settings_setstr)(fluid_settings_t * settings, const char * name, const char * val);
typedef int (WINAPIV * _fluid_settings_dupstr)(fluid_settings_t * settings, const char * name, char ** val);

typedef int (WINAPIV * _fluid_settings_get_type)(fluid_settings_t * settings, const char * name);

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

#define InitializeFunction(type, address) { address = (##_##type) ::GetProcAddress(_hModule, #type); if (*address == nullptr) throw midi::exception_t((std::string("Failed to link FluidSynth function: ") + TOSTRING(type)).c_str()); }

#pragma endregion

/// <summary>
/// Implements a FluidSynth driver.
/// </summary>
class FluidSynth
{
public:
    FluidSynth() : GetVersion(), _hModule()
    {
    }

    ~FluidSynth()
    {
        if (_hModule)
        {
            ::FreeLibrary(_hModule);
            _hModule = 0;
        }
    }

    bool IsInitialized() const noexcept { return (_hModule != 0); }

    void Initialize(const WCHAR * basePath)
    {
        if (IsInitialized())
            return;

        if (!::PathIsDirectoryW(basePath))
            throw midi::exception_t(midi::GetErrorMessage("Invalid FluidSynth directory", ::GetLastError()).c_str());

        BOOL Success = ::SetDllDirectoryW(basePath);

        if (!Success)
            throw midi::exception_t(midi::GetErrorMessage("Failed to add FluidSynth directory to the search path", ::GetLastError()).c_str());

        _hModule = ::LoadLibraryW(LibraryName);

        if (_hModule == 0)
            throw midi::exception_t(midi::GetErrorMessage("Failed to load FluidSynth library", ::GetLastError(), WideToUTF8(LibraryName).c_str()).c_str());

        #pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'xxx'
        InitializeFunction(fluid_version, GetVersion);

        InitializeFunction(new_fluid_settings, CreateSettings);

        InitializeFunction(fluid_settings_setnum, SetNumericSetting);
        InitializeFunction(fluid_settings_setint, SetIntegerSetting);
        InitializeFunction(fluid_settings_setstr, SetStringSetting);
        InitializeFunction(fluid_settings_dupstr, GetStringSetting);
    
        InitializeFunction(fluid_settings_get_type, GetSettingType);

        InitializeFunction(fluid_free, Free);
        InitializeFunction(fluid_settings_foreach, ForEachSetting);

        InitializeFunction(fluid_is_soundfont, IsSoundFont);

        InitializeFunction(delete_fluid_settings, DeleteSettings);

        InitializeFunction(new_fluid_synth, CreateSynthesizer);
        InitializeFunction(fluid_synth_add_sfloader, AddSoundFontLoader);
        InitializeFunction(fluid_synth_system_reset, ResetSynthesizer);
        InitializeFunction(delete_fluid_synth, DeleteSynthesizer);

        InitializeFunction(new_fluid_defsfloader, CreateSoundFontLoader);
        InitializeFunction(fluid_sfloader_set_callbacks, SetSoundFontLoaderCallbacks);
        InitializeFunction(fluid_synth_sfload, LoadSoundFont);
        InitializeFunction(fluid_synth_set_bank_offset, SetSoundFontBankOffset);

        InitializeFunction(fluid_synth_set_interp_method, SetInterpolationMethod);

        InitializeFunction(fluid_synth_noteon, NoteOn);
        InitializeFunction(fluid_synth_noteoff, NoteOff);
        InitializeFunction(fluid_synth_cc, ControlChange);
        InitializeFunction(fluid_synth_get_cc, GetControlChange);
        InitializeFunction(fluid_synth_sysex, SysEx);

        InitializeFunction(fluid_synth_pitch_bend, PitchBend);
        InitializeFunction(fluid_synth_get_pitch_bend, GetPitchBend);
        InitializeFunction(fluid_synth_pitch_wheel_sens, PitchWheelSensitivity);
        InitializeFunction(fluid_synth_get_pitch_wheel_sens, GetPitchWheelSensitivity);
        InitializeFunction(fluid_synth_program_change, ProgramChange);
        InitializeFunction(fluid_synth_channel_pressure, ChannelPressure);
        InitializeFunction(fluid_synth_key_pressure, KeyPressure);
        InitializeFunction(fluid_synth_bank_select, BankSelect);

        InitializeFunction(fluid_synth_set_reverb_group_roomsize, SetReverbRoomSize);
        InitializeFunction(fluid_synth_set_reverb_group_damp, SetReverbDamp);
        InitializeFunction(fluid_synth_set_reverb_group_width, SetReverbWidth);
        InitializeFunction(fluid_synth_set_reverb_group_level, SetReverbLevel);
        InitializeFunction(fluid_synth_set_reverb_on, SetReverb);

        InitializeFunction(fluid_synth_set_chorus_nr, SetChorusVoiceCount);
        InitializeFunction(fluid_synth_set_chorus_level, SetChorusLevel);
        InitializeFunction(fluid_synth_set_chorus_speed, SetChorusSpeed);
        InitializeFunction(fluid_synth_set_chorus_depth, SetChorusDepth);
        InitializeFunction(fluid_synth_set_chorus_type, SetChorusType);
        InitializeFunction(fluid_synth_set_chorus_on, SetChorus);

        InitializeFunction(fluid_synth_write_float, WriteFloat);

        InitializeFunction(fluid_synth_get_active_voice_count, GetActiveVoiceCount);

        InitializeFunction(fluid_synth_get_settings, GetSettings);

        InitializeFunction(fluid_set_log_function, SetLogFunction);

        #pragma warning(default: 4191)
    }

    static bool Exists() noexcept
    {
        try
        {
            FluidSynth().Initialize(pfc::wideFromUTF8(CfgFluidSynthDirectoryPath.get()));

            return true;
        }
        catch (midi::exception_t e)
        {
            console::print(STR_COMPONENT_BASENAME, " failed to initialize FluidSynth: ", e.what());

            return false;
        }
    }

public:
    _fluid_version GetVersion;

    _new_fluid_settings CreateSettings;

    _fluid_settings_setnum SetNumericSetting;
    _fluid_settings_setint SetIntegerSetting;
    _fluid_settings_setstr SetStringSetting;
    _fluid_settings_dupstr GetStringSetting;

    _fluid_settings_get_type GetSettingType;

    _fluid_settings_foreach ForEachSetting;

    _fluid_free Free;
    _delete_fluid_settings DeleteSettings;

    _fluid_is_soundfont IsSoundFont;
    
    _new_fluid_synth CreateSynthesizer;
    _fluid_synth_add_sfloader AddSoundFontLoader;
    _fluid_synth_system_reset ResetSynthesizer;
    _delete_fluid_synth DeleteSynthesizer;

    _new_fluid_defsfloader CreateSoundFontLoader;
    _fluid_sfloader_set_callbacks SetSoundFontLoaderCallbacks;
    _fluid_synth_sfload LoadSoundFont;
    _fluid_synth_set_bank_offset SetSoundFontBankOffset;

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

    static inline const WCHAR * LibraryName = L"libfluidsynth-3.dll";
};
