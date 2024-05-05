
/** $VER: ADLPlayer.h (2023.09.27) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <libADLMIDI/include/adlmidi.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member
class ADLPlayer : public MIDIPlayer
{
public:
    ADLPlayer();
    virtual ~ADLPlayer();

    enum
    {
        ADLMIDI_EMU_NUKED = 0,
        ADLMIDI_EMU_NUKED_174,
        ADLMIDI_EMU_DOSBOX
    };

    void SetCore(uint32_t);
    void SetBank(uint32_t);
    void SetChipCount(uint32_t);
    void Set4OpCount(uint32_t);
    void SetFullPanning(bool);
    void SetBankFilePath(pfc::string8 filePath);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) override;

private:
    struct ADL_MIDIPlayer * _Player[3];

    uint32_t _EmuCore;
    uint32_t _BankNumber;
    uint32_t _ChipCount;
    uint32_t _4OpCount;
    bool _FullPanning;
    pfc::string8 _BankFilePath;
};
#pragma warning(default: 4820) // x bytes padding added after data member
