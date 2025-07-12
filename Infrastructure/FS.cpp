
/** $VER: FS.cpp (2025.07.12) P. Stuer **/

#include "pch.h"

#include "FS.h"

#include "Encoding.h"
#include "Exception.h"

#include "Configuration.h"
#include "Log.h"

#include <shlwapi.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace FluidSynth
{

void API::Initialize(const WCHAR * basePath)
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
    
    InitializeFunction(fluid_settings_get_type, GetSettingType);

    InitializeFunction(fluid_settings_getnum_range, GetNumericSettingRange);
    InitializeFunction(fluid_settings_getint_range, GetIntegerSettingRange);

    InitializeFunction(fluid_settings_getnum, GetNumericSetting);
    InitializeFunction(fluid_settings_getint, GetIntegerSetting);
    InitializeFunction(fluid_settings_dupstr, GetStringSetting);

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

bool API::Exists() noexcept
{
    try
    {
        FluidSynth::API().Initialize(pfc::wideFromUTF8(CfgFluidSynthDirectoryPath.get()));

        return true;
    }
    catch (component::runtime_error e)
    {
        Log.AtError().Format(STR_COMPONENT_BASENAME " failed to initialize FluidSynth: %s", e.what());

        return false;
    }
}

/// <summary>
/// Loads a FluidSynth configuration file.
/// </summary>
bool Host::LoadConfig(FluidSynth::API & api, const fs::path & filePath, fluid_settings_t * Settings) noexcept
{
    if (filePath.empty() || (Settings == nullptr))
        return false;

    std::ifstream Stream(filePath);

    if (!Stream.is_open())
        return false;

    std::string Line;

    while (std::getline(Stream, Line))
    {
        std::stringstream ss(Line);

        if (Line.empty() || Line.starts_with('#'))
            continue;

        std::string Name;
        std::string Value;

        ss >> Name >> Value;

        if (Name.empty() || Value.empty())
            continue;

        auto Type = api.GetSettingType(Settings, Name.c_str());

        switch (Type)
        {
            case FLUID_NUM_TYPE: // Numeric (double)
            {
                try
                {
                    double Numeric = std::stod(Value.c_str()), Min, Max;

                    if (api.SetNumericSetting(Settings, Name.c_str(), Numeric) != FLUID_OK)
                    {
                        api.GetNumericSettingRange(Settings, Name.c_str(), &Min, &Max);

                        Log.AtWarn().Format(STR_COMPONENT_BASENAME " found invalid numeric value \"%s\" for setting \"%s\" in FluidSynth configuration file. Valid range is %.2f to %2f.", Value.c_str(), Name.c_str(), Min, Max);
                    }
                }
                catch (...)
                {
                    Log.AtWarn().Format(STR_COMPONENT_BASENAME " found invalid numeric value \"%s\" for setting \"%s\" in FluidSynth configuration file.", Value.c_str(), Name.c_str());
                }
                break;
            }

            case FLUID_INT_TYPE: // Integer
            {
                try
                {
                    int Integer = std::stoi(Value.c_str()), Min, Max;

                    if (api.SetIntegerSetting(Settings, Name.c_str(), Integer) != FLUID_OK)
                    {
                        api.GetIntegerSettingRange(Settings, Name.c_str(), &Min, &Max);

                        Log.AtWarn().Format(STR_COMPONENT_BASENAME " found invalid integer value \"%s\" for setting \"%s\" in FluidSynth configuration file. Valid range is %d to %d.", Value.c_str(), Name.c_str(), Min, Max);
                    }
                }
                catch (...)
                {
                    Log.AtWarn().Format(STR_COMPONENT_BASENAME " found invalid integer value \"%s\" for setting \"%s\" in FluidSynth configuration file.", Value.c_str(), Name.c_str());
                }
                break;
            }

            case FLUID_STR_TYPE: // String
            {
                if (api.SetStringSetting(Settings, Name.c_str(), Value.c_str()) != FLUID_OK)
                {
                    Log.AtWarn().Format(STR_COMPONENT_BASENAME " found invalid string value \"%s\" for setting \"%s\" in FluidSynth configuration file.", Value.c_str(), Name.c_str());
                }
                break;
            }

            case FLUID_SET_TYPE: // Set of values
            {
                break;
            }

            case FLUID_NO_TYPE:
            default:
                Log.AtWarn().Format(STR_COMPONENT_BASENAME " found unknown setting \"%s\" in FluidSynth configuration file.", Name.c_str());
        }
    }

    return true;
}

}
