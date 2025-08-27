
/** $VER: OPN Player (2025.08.27) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <Player.h>

#include <libOPNMIDI/repo/include/opnmidi.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

class OPNPlayer : public player_t
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

    void SetEmulatorCore(uint32_t);
    void SetBankNumber(uint32_t);
    void SetChipCount(uint32_t);
    void SetSoftPanning(bool) noexcept;
    void SetBankFilePath(const std::string & filePath) noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t);

    virtual uint8_t GetPortCount() const noexcept override { return _countof(_Devices); };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;

private:
    struct OPN2_MIDIPlayer * _Devices[1]; // LibOPNMIDI supports only 1 device per thread. [Issue 293](https://github.com/Wohlstand/libADLMIDI/issues/293).

    uint32_t _EmulatorCore;
    uint32_t _BankNumber;
    uint32_t _ChipCount;
    bool _IsSoftPanningEnabled;
    std::string _BankFilePath;
};

#pragma warning(default: 4820) // x bytes padding added after data member
