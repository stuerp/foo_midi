
/** $VER: MIDIPlayer.h (2023.01.15) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>

#include <midi_processing/midi_container.h>

#ifdef EXPERIMENT
#include <API.h>
#endif

#pragma warning(disable: 4820) // x bytes padding added after data member
class MIDIPlayer
{
public:
    MIDIPlayer();
    virtual ~MIDIPlayer() { };

    bool Load(const midi_container & midiContainer, unsigned int subsongIndex, unsigned int loopMode, unsigned int cleanFlags);
    unsigned long Play(audio_sample * samples, unsigned long samplesSize);
    void Seek(unsigned long seekTime);

    void SetSampleRate(unsigned long sampleRate);

    enum LoopMode
    {
        LoopModeEnabled = 0x01,
        LoopModeForced = 0x02
    };

    void SetLoopMode(LoopMode loopMode);

    enum FilterType
    {
        FilterNone = 0,
        FilterGMSysEx,
        FilterGM2SysEx,
        FilterSC55SysEx,
        FilterSC88SysEx,
        FilterSC88ProSysEx,
        FilterSC8850SysEx,
        FilterXGSysEx
    };

    void SetFilter(FilterType filterType, bool filterEffects);

    virtual bool GetErrorMessage(std::string &) { return false; }

protected:
    virtual bool Startup() { return false; }
    virtual void Shutdown() { };
    virtual void Render(audio_sample *, unsigned long) { }
    virtual bool Reset() { return false; }

    // Should return the block size that the player expects, otherwise 0.
    virtual unsigned int GetSampleBlockSize() { return 0; }

    virtual void SendEvent(uint32_t) { }
    virtual void SendSysEx(const uint8_t *, size_t, size_t) { };
    virtual void SendEventWithTime(uint32_t, unsigned int) { };
    virtual void SendSysExWithTime(const uint8_t *, size_t, size_t, unsigned int) { };

    void SendSysExReset(size_t port, unsigned int time);

protected:
    bool _IsInitialized;
    unsigned long _SampleRate;
    system_exclusive_table _SysExMap;

    FilterType _FilterType;
    bool _FilterEffects;

private:
    void SendEventFiltered(uint32_t event);
    void SendSysExFiltered(const uint8_t * event, size_t size, size_t port);

    void SendEventWithTimeFiltered(uint32_t b, size_t time);
    void SendSysExWithTimeFiltered(const uint8_t * event, size_t size, size_t port, size_t time);

    void SendSysExResetSC(size_t port, unsigned int time);
    void SendSysExGS(uint8_t * data, size_t size, size_t port, unsigned int time);

    std::vector<midi_stream_event> _Stream;

    size_t _CurrentPosition; // Current position in the MIDI stream
    size_t _CurrentTime;
    size_t _EndTime;
    size_t _SamplesRemaining;

    unsigned int _LoopMode;

    unsigned long _LoopStart;
    unsigned long _LoopEnd;
    unsigned long _LoopStartTime;

    #ifdef EXPERIMENT
    foo_vis_midi::IMusicKeyboard::ptr _MusicKeyboard;
    #endif
};
#pragma warning(default: 4820)
