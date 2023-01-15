
/** $VER: OPN Player (2023.01.04) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <MIDIPlayer.h>

#include <libOPNMIDI/include/opnmidi.h>

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
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long);

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, size_t) override;

private:
    struct OPN2_MIDIPlayer * _Player[3];

    unsigned _EmuCore;
    unsigned _BankNumber;
    unsigned _ChipCount;
    bool _FullPanning;
};
#pragma warning(default: 4820) // x bytes padding added after data member
