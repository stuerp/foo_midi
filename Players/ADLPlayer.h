
/** $VER: ADLPlayer.h (2025.06.22) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#include <libADLMIDI/include/adlmidi.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

class ADLPlayer : public player_t
{
public:
    ADLPlayer();
    virtual ~ADLPlayer();

    void SetEmulatorCore(uint32_t) noexcept;
    void SetBankNumber(uint32_t);
    void SetChipCount(uint32_t);
    void Set4OpChannelCount(uint32_t) noexcept;
    void SetSoftPanning(bool) noexcept;
    void SetBankFilePath(const std::string & filePath) noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) override;

private:
    struct ADL_MIDIPlayer * _Player[3];

    uint32_t _EmulatorCore;
    uint32_t _BankNumber;
    uint32_t _ChipCount;
    uint32_t _4OpChannelCount;
    bool _IsSoftPanningEnabled;
    std::string _BankFilePath;
};

#pragma warning(default: 4820) // x bytes padding added after data member
