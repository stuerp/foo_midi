
/** $VER: MT32Player.h (2024.09.29) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <Player.h>

#include <mt32emu.h>

namespace foobar2000_io
{
    class abort_callback;
};

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member
class MT32Player : public player_t
{
public:
    MT32Player(bool gm = false, unsigned gm_set = 0);
    virtual ~MT32Player();

    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept override
    {
        _AbortCallback = abortHandler;
    }

    void SetBasePath(const char * in);

    bool IsConfigValid();

    static int GetSampleRate();

protected:
    #pragma region("MIDIPlayer")
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override;
    #pragma endregion

private:
    void _reset();

    MT32Emu::File * openFile(const char * filename);

private:
    MT32Emu::Synth * _Synth;
    pfc::string8 _BasePathName;
    foobar2000_io::abort_callback * _AbortCallback;

    MT32Emu::File * controlRomFile, * pcmRomFile;
    const MT32Emu::ROMImage * controlRom, * pcmRom;

    unsigned int _GMSet;
    bool _IsGM;
};
#pragma warning(default: 4820) // x bytes padding added after data member
