
/** $VER: FS.cpp (2025.07.01) P. Stuer **/

#include "pch.h"

#include "FS.h"

#include "Encoding.h"
#include "Exception.h"

#include "Configuration.h"

#include <shlwapi.h>

void FluidSynth::Initialize(const WCHAR * basePath)
{
    if (IsInitialized())
        return;

    if (!::PathIsDirectoryW(basePath))
        throw component::runtime_error(component::GetErrorMessage("Invalid FluidSynth directory", ::GetLastError()));

    BOOL Success = ::SetDllDirectoryW(basePath);

    if (!Success)
        throw component::runtime_error(component::GetErrorMessage("Failed to add FluidSynth directory to the search path", ::GetLastError()));

    _hModule = ::LoadLibraryW(LibraryName);

    if (_hModule == 0)
        throw component::runtime_error(component::GetErrorMessage("Failed to load FluidSynth library \"%s\"", ::GetLastError(), ::WideToUTF8(LibraryName).c_str()));

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

bool FluidSynth::Exists() noexcept
{
    try
    {
        FluidSynth().Initialize(pfc::wideFromUTF8(CfgFluidSynthDirectoryPath.get()));

        return true;
    }
    catch (component::runtime_error e)
    {
        console::print(STR_COMPONENT_BASENAME, " failed to initialize FluidSynth: ", e.what());

        return false;
    }
}
