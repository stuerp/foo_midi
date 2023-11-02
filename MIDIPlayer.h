
/** $VER: MIDIPlayer.h (2023.11.01) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <libmidi/MIDIContainer.h>
#include "Configuration.h"

#ifdef EXPERIMENT
#include <API.h>
#endif

#pragma warning(disable: 4820) // x bytes padding added after data member
class MIDIPlayer
{
public:
    MIDIPlayer();
    virtual ~MIDIPlayer() { };

    enum LoopMode
    {
        None = 0x00,
        Enabled = 0x01,
        Forced = 0x02
    };

    bool Load(const MIDIContainer & midiContainer, uint32_t subsongIndex, LoopType loopMode, uint32_t cleanFlags);
    uint32_t Play(audio_sample * samples, uint32_t samplesSize) noexcept;
    void Seek(uint32_t seekTime);

    void SetSampleRate(uint32_t sampleRate);

    void Configure(MIDIFlavor midiFlavor, bool filterEffects);

    virtual uint32_t GetChannelCount() const noexcept { return 2; }
    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept { UNREFERENCED_PARAMETER(abortHandler); }
    virtual bool GetErrorMessage(std::string &) { return false; }

protected:
    virtual bool Startup() { return false; }
    virtual void Shutdown() { };
    virtual void Render(audio_sample *, uint32_t) { }
    virtual bool Reset() { return false; }

    // Should return the block size that the player expects, otherwise 0.
    virtual uint32_t GetSampleBlockSize() const noexcept { return 0; }

    virtual void SendEvent(uint32_t) { }
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) { };

    // Only implemented by Secret Sauce and VSTi-specific
    virtual void SendEvent(uint32_t, uint32_t) { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t) { };

    void SendSysExReset(uint8_t portNumber, uint32_t time);

    uint32_t GetProcessorArchitecture(const std::string & filePath) const;

protected:
    bool _IsInitialized;
    uint32_t _SampleRate;
    SysExTable _SysExMap;

    MIDIFlavor _MIDIFlavor;
    bool _FilterEffects;

private:
    void SendEventFiltered(uint32_t data);
    void SendEventFiltered(uint32_t data, uint32_t time);

    void SendSysExFiltered(const uint8_t * event, size_t size, uint8_t portNumber);
    void SendSysExFiltered(const uint8_t * event, size_t size, uint8_t portNumber, uint32_t time);

    void SendSysExSetToneMapNumber(uint8_t portNumber, uint32_t time);
    void SendSysExGS(uint8_t * data, size_t size, uint8_t portNumber, uint32_t time);

private:
    std::vector<MIDIStreamEvent> _Stream;
    size_t _StreamPosition; // Current position in the event stream

    uint32_t _Position;     // Current position in the sample stream
    uint32_t _Length;       // Total length of the sample stream
    uint32_t _Remainder;    // In samples

    LoopType _LoopType;

    uint32_t _StreamLoopBegin;
    uint32_t _StreamLoopEnd;

    uint32_t _LoopBegin;    // Position of the start of a loop in the sample stream

    #ifdef EXPERIMENT
    foo_vis_midi::IMusicKeyboard::ptr _MusicKeyboard;
    #endif
};
#pragma warning(default: 4820)
