
/** $VER: OPN Player (2023.08.19) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <MIDIPlayer.h>

#include <libOPNMIDI/include/opnmidi.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
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

    void setCore(uint32_t);
    void setBank(uint32_t);
    void setChipCount(uint32_t);
    void setFullPanning(bool);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t);

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

private:
    struct OPN2_MIDIPlayer * _Player[3];

    unsigned _EmuCore;
    unsigned _BankNumber;
    unsigned _ChipCount;
    bool _FullPanning;
};
#pragma warning(default: 4820) // x bytes padding added after data member
