#ifndef __MT32Player_h__
#define __MT32Player_h__

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include <MIDIPlayer.h>

#include <mt32emu.h>

namespace foobar2000_io
{
    class abort_callback;
};

class MT32Player : public MIDIPlayer
{
public:
    MT32Player(bool gm = false, unsigned gm_set = 0);
    virtual ~MT32Player();

    void setBasePath(const char * in);
    void setAbortCallback(foobar2000_io::abort_callback * in);

    bool isConfigValid();

    static int getSampleRate();

protected:
    virtual void send_event(uint32_t b);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

private:
    void _reset();

    MT32Emu::File * openFile(const char * filename);

private:
    MT32Emu::Synth * _Synth;
    pfc::string8 _BasePathName;
    foobar2000_io::abort_callback * _AbortCallback;

    MT32Emu::File * controlRomFile, * pcmRomFile;
    const MT32Emu::ROMImage * controlRom, * pcmRom;

    unsigned uGMSet;
    bool bGM;
};

#endif
