
/** $VER: MT32Player.h (2013.01.15) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include <MIDIPlayer.h>

#include <mt32emu.h>

namespace foobar2000_io
{
    class abort_callback;
};

#pragma warning(disable: 4820) // x bytes padding added after data member
class MT32Player : public MIDIPlayer
{
public:
    MT32Player(bool gm = false, unsigned gm_set = 0);
    virtual ~MT32Player();

    void setBasePath(const char * in);
    void setAbortCallback(foobar2000_io::abort_callback * in);

    bool isConfigValid();

    static int GetSampleRate();

protected:
    #pragma region("MIDIPlayer")
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * out, unsigned long count) override;

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;
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
