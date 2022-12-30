#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "../MIDIPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class OPNPlayer : public MIDIPlayer
{
public:
    OPNPlayer();
    virtual ~OPNPlayer();

    enum
    {
        OPNMIDI_EMU_MAME = 0,   // MAME YM2612
        OPNMIDI_EMU_NUKED,      // Nuked OPN2
        OPNMIDI_EMU_GENS,       // GENS
    };

    void setCore(unsigned);
    void setBank(unsigned);
    void setChipCount(unsigned);
    void setFullPanning(bool);

protected:
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample *, unsigned long);

    virtual void send_event(uint32_t) override;
    virtual void send_sysex(const uint8_t *, size_t, size_t) override;

private:
    struct OPN2_MIDIPlayer * _Player[3];

    unsigned _EmuCore;
    unsigned _BankNumber;
    unsigned _ChipCount;
    bool _FullPanning;
};
#pragma warning(default: 4820) // x bytes padding added after data member
