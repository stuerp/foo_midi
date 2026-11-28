
/** $VER: Player.h (2026.11.28) P. Stuer **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <MIDIContainer.h>
#include <SysEx.h>

#include "Configuration.h"

#ifdef HAVE_FOO_VIS_MIDI
#include <API.h>
#endif

#pragma warning(disable: 4820) // x bytes padding added after data member

class player_t
{
public:
    player_t() = default;

    virtual ~player_t() = default;

    bool Load(const midi::container_t & container, uint32_t subsongIndex, LoopType loopMode, uint32_t decayTime, uint32_t cleanFlags);
    uint32_t Play(audio_sample * frameData, uint32_t frameCount);
    void Seek(uint32_t newFrameIndex);

    void EnableVisualization(bool enabled) noexcept;

    uint32_t GetSampleRate() const noexcept { return _SampleRate; };
    void SetSampleRate(uint32_t sampleRate);

    void Configure(MIDIFlavor midiFlavor, bool filterEffects);

    virtual uint32_t GetAudioChannelCount() const noexcept { return 2; } // Gets the number of audio channels the player supports.
    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept { UNREFERENCED_PARAMETER(abortHandler); }
    
    const std::string & GetErrorMessage() const noexcept { return _ErrorMessage; };

protected:
    virtual bool Startup() { return false; }
    virtual void Shutdown() { };
    virtual void Render(audio_sample *, uint32_t) { }
    virtual bool Reset() { return false; }

    // Should return the block size (in no. of frames) that the player expects, otherwise 0.
    virtual uint32_t GetBlockSize() const noexcept { return 0; }

    virtual uint8_t GetPortCount() const noexcept = 0;

    virtual void SendEvent(uint32_t) noexcept { }
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) { };

    // Only implemented by Secret Sauce, VSTi and CLAP
    virtual void SendEvent(uint32_t, uint32_t time) { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t time) { };

    void ResetPort(uint8_t portNumber, uint32_t time);

    uint32_t GetProcessorArchitecture(const fs::path & filePath) const;

protected:
    bool _IsStarted = false;
    uint32_t _SampleRate = 1'000;
    midi::sysex_table_t _SysExMap;
    std::vector<uint8_t> _PortNumbers;

    MIDIFlavor _MIDIFlavor = MIDIFlavor::Default;
    bool _FilterEffects = false;

    std::string _ErrorMessage;

    std::vector<midi::message_t> _Messages;
    midi::FileFormat _FileFormat = midi::FileFormat::Unknown;

private:
    void SendEventFiltered(uint32_t data);
    void SendEventFiltered(uint32_t data, uint32_t time);

    bool FilterEvent(uint32_t data) noexcept;
    bool FilterEffect(uint32_t data) const noexcept;

    void SendSysExFiltered(const uint8_t * event, size_t size, uint8_t portNumber);
    void SendSysExFiltered(const uint8_t * event, size_t size, uint8_t portNumber, uint32_t time);

    void SendSysExSetToneMapNumber(uint8_t portNumber, uint32_t time);
    void SendSysExGS(uint8_t * data, size_t size, uint8_t portNumber, uint32_t time);

private:
    LoopType _LoopType              = LoopType::NeverLoop;  // Type of looping requested by the user.
    uint32_t _DecayTime             = 0;    // in ms

    uint32_t _MessageIndex          = 0;    // Current MIDI message
    uint32_t _MessageIndexLoopBegin = 0;    // Start of the loop in the event stream
    uint32_t _LoopEndIndex          = 0;    // End of the loop in the event stream

    uint32_t _FrameIndex            = 0;    // Current frame in the sample stream
    uint32_t _FrameCount            = 0;    // Number of frames in the sample stream
    uint32_t _FramesRemaining       = 0;    // Number of frames that still need to be rendered before the audio chunk is complete (in case the block size of the player is smaller than the audio chunk size).

    uint32_t _FrameIndexLoopBegin   = 0;    // Frame index in the sample stream of the start of the loop

    uint64_t _ChannelsMaskVersion   = 0;  // Version number of the channel configuration
    uint16_t _ChannelsMask[128] { };

#ifdef HAVE_FOO_VIS_MIDI
    IMusicKeyboard::ptr _MusicKeyboard;
    IMusicKeyboard2::ptr _MusicKeyboard2;
#endif
};

#pragma warning(default: 4820)
