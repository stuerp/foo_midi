
/** $VER: FluidSynth.h (2023.08.02) **/

#pragma once

#include "MIDIPlayer.h"

#include <fluidsynth.h>

#include <pfc/string-conv-lite.h>

#pragma region FluidSynth API

typedef char * (WINAPIV * _fluid_version_str)();

typedef fluid_settings_t * (WINAPIV * _new_fluid_settings)();
typedef int (WINAPIV * _fluid_settings_setnum)(fluid_settings_t * settings, const char * name, double val);
typedef int (WINAPIV * _fluid_settings_setint)(fluid_settings_t * settings, const char * name, int val);
typedef void (WINAPIV * _delete_fluid_settings)(fluid_settings_t * settings);

typedef fluid_synth_t * (WINAPIV * _new_fluid_synth)(fluid_settings_t * settings);
typedef void (WINAPIV * _fluid_synth_add_sfloader)(fluid_synth_t * synth, fluid_sfloader_t * loader);
typedef int (WINAPIV * _fluid_synth_system_reset)(fluid_synth_t *synth);
typedef void (WINAPIV * _delete_fluid_synth)(fluid_synth_t * synth);

typedef fluid_sfloader_t * (WINAPIV * _new_fluid_defsfloader)(fluid_settings_t * settings);
typedef int (WINAPIV * _fluid_sfloader_set_callbacks)(fluid_sfloader_t * loader, fluid_sfloader_callback_open_t open, fluid_sfloader_callback_read_t read, fluid_sfloader_callback_seek_t seek, fluid_sfloader_callback_tell_t tell, fluid_sfloader_callback_close_t close);

typedef int (WINAPIV * _fluid_synth_set_interp_method)(fluid_synth_t * synth, int chan, int interp_method);

typedef void (WINAPIV * _fluid_synth_add_sfloader)(fluid_synth_t * synth, fluid_sfloader_t * loader);
typedef int (WINAPIV * _fluid_synth_sfload)(fluid_synth_t * synth, const char *filename, int reset_presets);

typedef int (WINAPIV * _fluid_synth_noteon)(fluid_synth_t * synth, int chan, int key, int vel);
typedef int (WINAPIV * _fluid_synth_noteoff)(fluid_synth_t * synth, int chan, int key);
typedef int (WINAPIV * _fluid_synth_cc)(fluid_synth_t * synth, int chan, int ctrl, int val);
typedef int (WINAPIV * _fluid_synth_get_cc)(fluid_synth_t * synth, int chan, int ctrl, int *pval);
typedef int (WINAPIV * _fluid_synth_sysex)(fluid_synth_t * synth, const char * data, int len, char * response, int * response_len, int * handled, int dryrun);

typedef int (WINAPIV * _fluid_synth_pitch_bend)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_get_pitch_bend)(fluid_synth_t * synth, int chan, int *ppitch_bend);
typedef int (WINAPIV * _fluid_synth_pitch_wheel_sens)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_get_pitch_wheel_sens)(fluid_synth_t * synth, int chan, int *pval);
typedef int (WINAPIV * _fluid_synth_program_change)(fluid_synth_t * synth, int chan, int program);
typedef int (WINAPIV * _fluid_synth_channel_pressure)(fluid_synth_t * synth, int chan, int val);
typedef int (WINAPIV * _fluid_synth_key_pressure)(fluid_synth_t * synth, int chan, int key, int val);
typedef int (WINAPIV * _fluid_synth_bank_select)(fluid_synth_t * synth, int chan, int bank);

typedef int (WINAPIV * _fluid_synth_write_float)(fluid_synth_t * synth, int len, void * lout, int loff, int lincr, void * rout, int roff, int rincr);

typedef int (WINAPIV * _fluid_synth_get_active_voice_count)(fluid_synth_t * synth);

#define InitializeFunction(type, address) { address = (##_##type) ::GetProcAddress(_hModule, #type); if (*address == nullptr) return false; }

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

    bool Initialize(const WCHAR * basePath) noexcept
    {
        if (IsInitialized())
            return true;

        BOOL Success = ::SetDllDirectoryW(basePath);

        if (!Success)
            return false;

        _hModule = ::LoadLibraryW(LibraryName);

        if (_hModule == 0)
            return false;

        #pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC' to 'xxx'
        InitializeFunction(fluid_version_str, GetVersion);

        InitializeFunction(new_fluid_settings, CreateSettings);

        InitializeFunction(fluid_settings_setnum, SetNumericSetting);
        InitializeFunction(fluid_settings_setint, SetIntegerSetting);
        InitializeFunction(delete_fluid_settings, DeleteSettings);

        InitializeFunction(new_fluid_synth, CreateSynthesizer);
        InitializeFunction(fluid_synth_add_sfloader, AddSoundFontLoader);
        InitializeFunction(fluid_synth_system_reset, ResetSynthesizer);
        InitializeFunction(delete_fluid_synth, DeleteSynthesizer);

        InitializeFunction(new_fluid_defsfloader, CreateSoundFontLoader);
        InitializeFunction(fluid_sfloader_set_callbacks, SetSoundFontLoaderCallbacks);
        InitializeFunction(fluid_synth_sfload, LoadSoundFont);

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

        InitializeFunction(fluid_synth_write_float, WriteFloat);

        InitializeFunction(fluid_synth_get_active_voice_count, GetActiveVoiceCount);

        #pragma warning(default: 4191)

        return true;
    }

    static bool Exists() noexcept
    {
        return FluidSynth().Initialize(pfc::wideFromUTF8(CfgFluidSynthDirectoryPath.get()));
    }

public:
    _fluid_version_str GetVersion;

    _new_fluid_settings CreateSettings;
    _fluid_settings_setnum SetNumericSetting;
    _fluid_settings_setint SetIntegerSetting;
    _delete_fluid_settings DeleteSettings;

    _new_fluid_synth CreateSynthesizer;
    _fluid_synth_add_sfloader AddSoundFontLoader;
    _fluid_synth_system_reset ResetSynthesizer;
    _delete_fluid_synth DeleteSynthesizer;

    _new_fluid_defsfloader CreateSoundFontLoader;
    _fluid_sfloader_set_callbacks SetSoundFontLoaderCallbacks;
    _fluid_synth_sfload LoadSoundFont;

    _fluid_synth_set_interp_method SetInterpolationMethod;

    _fluid_synth_noteon NoteOn;
    _fluid_synth_noteoff NoteOff;
    _fluid_synth_cc ControlChange;
    _fluid_synth_get_cc GetControlChange;
    _fluid_synth_sysex SysEx;

    _fluid_synth_pitch_bend PitchBend;
    _fluid_synth_get_pitch_bend GetPitchBend;
    _fluid_synth_pitch_wheel_sens PitchWheelSensitivity;
    _fluid_synth_get_pitch_wheel_sens GetPitchWheelSensitivity;
    _fluid_synth_program_change ProgramChange;
    _fluid_synth_channel_pressure ChannelPressure;
    _fluid_synth_key_pressure KeyPressure;
    _fluid_synth_bank_select BankSelect;

    _fluid_synth_write_float WriteFloat;

    _fluid_synth_get_active_voice_count GetActiveVoiceCount;

private:
    HMODULE _hModule;

    static inline const WCHAR * LibraryName = L"libfluidsynth-3.dll";
};
