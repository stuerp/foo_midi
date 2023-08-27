
/** $VER: MIDIPlayer.h (2023.08.27) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <libmidi/MIDIContainer.h>

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
        LoopModeEnabled = 0x01,
        LoopModeForced = 0x02
    };

    bool Load(const MIDIContainer & midiContainer, uint32_t subsongIndex, LoopMode loopMode, uint32_t cleanFlags);
    uint32_t Play(audio_sample * samples, uint32_t samplesSize) noexcept;
    void Seek(uint32_t seekTime);

    void SetSampleRate(uint32_t sampleRate);

    void SetLoopMode(LoopMode loopMode);

    enum class ConfigurationType
    {
        None = 0,
        GM,
        GM2,
        SC55,
        SC88,
        SC88Pro,
        SC8850,
        XG
    };

    void Configure(ConfigurationType filterType, bool filterEffects);

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

    ConfigurationType _ConfigurationType;
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
    size_t _CurrentPosition; // Current position in the MIDI stream

    uint32_t _TimeInSamples;
    uint32_t _TimeInSamplesRemaining;
    uint32_t _LengthInSamples;

    uint32_t _LoopMode;

    uint32_t _LoopBegin;
    uint32_t _LoopBeginTime;
    uint32_t _LoopEnd;

    #ifdef EXPERIMENT
    foo_vis_midi::IMusicKeyboard::ptr _MusicKeyboard;
    #endif
};
#pragma warning(default: 4820)
