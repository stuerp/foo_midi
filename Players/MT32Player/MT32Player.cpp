
/** $VER: MT32Player.cpp (2025.07.13) **/

#include "pch.h"

#include "MT32Player.h"
#include "MT32SetupGM.h"
#include "MT32SetupGMKQ6.h"

#include "Configuration.h"
#include "Resource.h"
#include "Log.h"

#pragma warning(disable: 5045)

#include <pfc/pathUtils.h>

MT32Player::MT32Player(bool isMT32, uint32_t gmSet) : player_t(), _IsMT32(isMT32), _GMSet(gmSet)
{
}

MT32Player::~MT32Player()
{
    Shutdown();
}

#pragma region player_t

bool MT32Player::Startup()
{
    if (_IsStarted)
        return true;

    _Service.createContext(_ReportHandler);

    if (!LoadROMs(""))
        return false;

    _Service.setStereoOutputSampleRate(0);
    _Service.setSamplerateConversionQuality((MT32Emu::SamplerateConversionQuality) (int64_t) CfgMT32EmuConversionQuality);
    _Service.setPartialCount((MT32Emu::Bit32u) (int64_t) CfgMT32EmuMaxPartials);
    _Service.setAnalogOutputMode((MT32Emu::AnalogOutputMode) (int64_t) CfgMT32EmuAnalogOutputMode);

    _Service.selectRendererType(MT32Emu::RendererType::RendererType_FLOAT);

    if (_Service.openSynth() != MT32EMU_RC_OK)
        return false;

    _Service.setDACInputMode((MT32Emu::DACInputMode) (int64_t) CfgMT32EmuDACInputMode);

    _Service.setReverbEnabled((bool) CfgMT32EmuReverb);
    _Service.setNiceAmpRampEnabled((bool) CfgMT32EmuNiceAmpRamp);
    _Service.setNicePanningEnabled((bool) CfgMT32EmuNicePanning);
    _Service.setNicePartialMixingEnabled((bool) CfgMT32EmuNicePartialMixing);
    _Service.setReversedStereoEnabled((bool) CfgMT32EmuReverseStereo); // Early MT-32 units (especially those with ROM version 1.07 or earlier) had reversed stereo channels.

    MT32Emu::Bit32u ActualSampleRate = _Service.getActualStereoOutputSamplerate();

    SetSampleRate((uint32_t) ActualSampleRate);

    Reset();

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using LibMT32Emu %s.", _Service.getLibraryVersionString());

    _IsStarted = true;

    return true;
}

void MT32Player::Shutdown()
{
    _Service.freeContext();

    _IsStarted = false;
}

void MT32Player::Render(audio_sample * sampleData, uint32_t sampleCount)
{
/*
    MT32Emu::Bit16s Data[256 * 2];

    while (sampleCount != 0)
    {
        uint32_t ToDo = sampleCount;

        if (ToDo > 256)
            ToDo = 256;

        _Service.renderBit16s(Data, ToDo);

        audio_math::convert_from_int16(Data, ((size_t) ToDo * 2), sampleData, 1.);

        sampleData += (ToDo * 2);
        sampleCount -= ToDo;
    }
*/
    float Data[256 * 2];

    while (sampleCount != 0)
    {
        uint32_t ToDo = sampleCount;

        if (ToDo > 256)
            ToDo = 256;

        _Service.renderFloat(Data, ToDo);

        for (uint32_t i = 0; i < (size_t) ToDo * 2; ++i)
            sampleData[i] = Data[i];

        sampleData += (ToDo * 2);
        sampleCount -= ToDo;
    }
}

bool MT32Player::Reset()
{
    _Service.playSysexNow(midi::sysex_t::MT32Reset, sizeof(midi::sysex_t::MT32Reset));

    if (_IsMT32)
        return true;

    // Send the Roland or King's Quest 6 GM configuration.
    const uint8_t * Begin, * End;

    if (_GMSet == 0)
    {
        Begin = MT32GMSysEx;
        End = Begin + _countof(MT32GMSysEx);
    }
    else
    {
        Begin = MT32GMKQ6SysEx;
        End = Begin + _countof(MT32GMKQ6SysEx);
    }

    while (Begin < End)
    {
        const uint8_t * EndOfSysEx = Begin;

        while ((EndOfSysEx < End) && (*EndOfSysEx != 0xF7))
            EndOfSysEx++;

        _Service.playSysexNow(Begin, (MT32Emu::Bit32u)(EndOfSysEx - Begin + 1));

        Begin = EndOfSysEx + 1;
    }

    return true;
}

void MT32Player::SendEvent(uint32_t data)
{
    _Service.playMsg(data);
}

void MT32Player::SendSysEx(const uint8_t * data, size_t size, uint32_t)
{
    _Service.playSysex(data, (MT32Emu::Bit32u) size);
}

#pragma endregion

void MT32Player::SetROMDirectory(const fs::path & directoryPath) noexcept
{
    _ROMDirectory = directoryPath;

    Shutdown();
}

uint32_t MT32Player::GetSampleRate() noexcept
{
    return _Service.getActualStereoOutputSamplerate();
}
/*
/// <summary>
/// Forces the player to startup. This is necessary because we need the real sample rate before we actually start rendering frames.
/// </summary>
bool MT32Player::IsConfigValid() noexcept
{
    return Startup();
}
*/
/// <summary>
/// Loads the ROMs for the specified machine.
/// </summary>
bool MT32Player::LoadROMs(const std::string & machineID) noexcept
{
    auto MachineIDs = GetMatchingMachineIDs(machineID);

    if (!MachineIDs.empty())
        return LoadROMs(MachineIDs);

    return LoadMachineROMs(machineID);
}

/// <summary>
/// Gets the machine IDs that match the filter.
/// </summary>
std::vector<std::string> MT32Player::GetMatchingMachineIDs(const std::string & machineID)
{
    std::vector<std::string> MachineIDs;

    const bool AnyMachine = machineID.empty();

    if (!(AnyMachine || (machineID == "mt32") || (machineID == "cm32l")))
        return MachineIDs;

    const size_t KnownMachineCount = _Service.getMachineIDs(nullptr, 0);

    const char ** KnownMachineIDs = new const char * [KnownMachineCount];

    if (KnownMachineIDs == nullptr)
        return MachineIDs;

    _Service.getMachineIDs(KnownMachineIDs, KnownMachineCount);

    for (size_t i = 0; i < KnownMachineCount; ++i)
        if (AnyMachine || (std::strncmp(machineID.c_str(), KnownMachineIDs[i], 4) == 0))
            MachineIDs.push_back(KnownMachineIDs[i]);

    delete[] KnownMachineIDs;

    return MachineIDs;
}

/// <summary>
/// Loads the ROMs for any of the specified machines.
/// </summary>
bool MT32Player::LoadROMs(const std::vector<std::string> & machineIDs) noexcept
{
    bool ROMsLoaded = false;

    auto ROMs = IdentifyControlROMs();

    if (ROMs.empty())
        return false;

    for (const auto & MachineID : machineIDs)
    {
        size_t Count = _Service.getROMIDs(nullptr, 0, MachineID.c_str());

        const char ** MachineROMIDs = new const char * [Count];

        _Service.getROMIDs(MachineROMIDs, Count, MachineID.c_str());

        if (MachineROMIDs != nullptr)
        {
            for (size_t i = 0; !ROMsLoaded && (i < Count); ++i)
            {
                if (ROMs.contains(MachineROMIDs[i]))
                {
                    if (LoadMachineROMs(MachineID))
                    {
                        ROMsLoaded = true;
                        break;
                    }
                }
            }

            delete[] MachineROMIDs;
        }

        if (ROMsLoaded)
            break;
    }

    return ROMsLoaded;
}

/// <summary>
/// Loads the ROMs of the specified machine.
/// </summary>
bool MT32Player::LoadMachineROMs(const std::string & machineID)
{
    bool FoundControlROM = false;
    bool FoundPCMROM = false;

    try
    {
        for (const auto & Entry : fs::directory_iterator(_ROMDirectory))
        {
            Log.AtTrace().Write(STR_COMPONENT_BASENAME " is identifying %s as %s.", machineID.c_str(), (const char *) Entry.path().u8string().c_str());

            auto rc = _Service.addMachineROMFile(machineID.c_str(), (const char *) Entry.path().u8string().c_str());

            if (rc == MT32EMU_RC_MACHINE_NOT_IDENTIFIED)
                continue;

            FoundControlROM = FoundControlROM || (rc == MT32EMU_RC_ADDED_CONTROL_ROM);
            FoundPCMROM = FoundPCMROM || (rc == MT32EMU_RC_ADDED_PCM_ROM);

            if (FoundControlROM && FoundPCMROM)
                return true;
        }
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " fails to load the machine ROMs: %s", e.what());
    }

    return false;
}

/// <summary>
/// Gets the list of ROM IDs for which a ROM image file can be found.
/// </summary>
std::set<std::string> MT32Player::IdentifyControlROMs()
{
    std::set<std::string> ROMs;

    mt32emu_rom_info ROMInfo = { };

    try
    {
        for (const auto & Entry : fs::directory_iterator(_ROMDirectory))
        {
            if ((_Service.identifyROMFile(&ROMInfo, (const char *) Entry.path().u8string().c_str(), nullptr) == MT32EMU_RC_OK) && (ROMInfo.control_rom_id != nullptr))
                ROMs.insert(ROMInfo.control_rom_id);
        }
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " fails to identify the control ROMs: %s", e.what());
    }

    return ROMs;
}
