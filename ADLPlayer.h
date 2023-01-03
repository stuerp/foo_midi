
/** $VER: ADLPlayer.h (2022.01.02) **/

#pragma once

#pragma warning(disable: 5045)

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <libADLMIDI/include/adlmidi.h>

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

    void setCore(unsigned);
    void setBank(unsigned);
    void setChipCount(unsigned);
    void setFullPanning(bool);
    void set4OpCount(unsigned);

protected:
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample *, unsigned long) override;

    virtual void send_event(uint32_t) override;
    virtual void send_sysex(const uint8_t *, size_t, size_t) override;

private:
    struct ADL_MIDIPlayer * _Player[3];

    unsigned _EmuCore;
    unsigned _BankNumber;
    unsigned _ChipCount;
    unsigned _4OpCount;
    bool _FullPanning;
};
#pragma warning(default: 4820) // x bytes padding added after data member
