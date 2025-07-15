
/** $VER: Player.h (2025.07.09) P. Stuer **/

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
    player_t() noexcept;

    virtual ~player_t() { };

    enum LoopMode
    {
        None = 0x00,
        Enabled = 0x01,
        Forced = 0x02
    };

    bool Load(const midi::container_t & midiContainer, uint32_t subsongIndex, LoopType loopMode, uint32_t decayTime, uint32_t cleanFlags);
    uint32_t Play(audio_sample * samplesData, uint32_t samplesSize) noexcept;
    void Seek(uint32_t seekTime);

    uint32_t GetSampleRate() const noexcept { return _SampleRate; };
    void SetSampleRate(uint32_t sampleRate);

    void Configure(MIDIFlavor midiFlavor, bool filterEffects);

    virtual uint32_t GetAudioChannelCount() const noexcept { return 2; } // Gets the number of audio channels the player supports.
    virtual void SetAbortHandler(foobar2000_io::abort_callback * abortHandler) noexcept { UNREFERENCED_PARAMETER(abortHandler); }
    
    std::string GetErrorMessage() const noexcept { return _ErrorMessage; };

protected:
    virtual bool Startup() { return false; }
    virtual void Shutdown() { };
    virtual void Render(audio_sample *, uint32_t) { }
    virtual bool Reset() { return false; }

    // Should return the block size (in no. of frames) that the player expects, otherwise 0.
    virtual uint32_t GetBlockSize() const noexcept { return 0; }

    virtual uint8_t GetPortCount() const noexcept = 0;

    virtual void SendEvent(uint32_t) { }
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) { };

    // Only implemented by Secret Sauce, VSTi and CLAP
    virtual void SendEvent(uint32_t, uint32_t time) { };
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t time) { };

    void ResetPort(uint8_t portNumber, uint32_t time);

    uint32_t GetProcessorArchitecture(const fs::path & filePath) const;

protected:
    bool _IsStarted;
    uint32_t _SampleRate;
    midi::sysex_table_t _SysExMap;
    std::vector<uint8_t> _Ports;

    MIDIFlavor _MIDIFlavor;
    bool _FilterEffects;

    std::string _ErrorMessage;

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
    LoopType _LoopType;             // Type of looping requested by the user.
    uint32_t _DecayTime;            // in ms

    std::vector<midi::message_t> _Messages;

    uint32_t _MessageIndex;         // Current MIDI message
    uint32_t _LoopBeginIndex;       // Start of the loop in the event stream
    uint32_t _LoopEndIndex;         // End of the loop in the event stream

    uint32_t _Time;                 // Current time in the sample stream (in ms)
    uint32_t _TotalTime;            // Total length of the sample stream (in ms)
    uint32_t _LoopBeginTime;        // Timestamp of the start of the loop in the sample stream (in ms)

    uint32_t _TimeRemaining;        // Remaining number of samples that need to be rendered before the audio chunk is complete (in case the block size of the player is smaller than the audio chunk size).

    uint64_t _ChannelsMaskVersion;  // Version number of the channel configuration
    uint16_t _ChannelsMask[128];

#ifdef HAVE_FOO_VIS_MIDI
    IMusicKeyboard::ptr _MusicKeyboard;
#endif
};

#pragma warning(default: 4820)
