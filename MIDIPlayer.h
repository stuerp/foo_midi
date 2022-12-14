#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include <midi_processing/midi_container.h>

#include <foobar2000.h>

class MIDIPlayer
{
public:
    enum
    {
        loop_mode_enable = 1 << 0,
        loop_mode_force = 1 << 1
    };

    typedef enum
    {
        filter_default = 0,
        filter_gm,
        filter_gm2,
        filter_sc55,
        filter_sc88,
        filter_sc88pro,
        filter_sc8850,
        filter_xg
    } filter_mode;

    MIDIPlayer();

    virtual ~MIDIPlayer() { };

    // setup
    void setSampleRate(unsigned long rate);
    void setLoopMode(unsigned int mode);
    void setFilterMode(filter_mode m, bool disable_reverb_chorus);

    bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
    unsigned long Play(audio_sample * out, unsigned long count);
    void Seek(unsigned long sample);

    bool GetLastError(std::string & p_out);

protected:
    // this should return the block size that the renderer expects, otherwise 0
    virtual unsigned int send_event_needs_time()
    {
        return 0;
    }
    virtual void send_event(uint32_t) { }
    virtual void send_sysex(const uint8_t *, size_t, size_t) { };
    virtual void render(audio_sample *, unsigned long) { }

    virtual void shutdown()
    {
    };
    virtual bool startup()
    {
        return false;
    }
    virtual bool reset()
    {
        return false;
    }

    virtual bool get_last_error(std::string &)
    {
        return false;
    }

    // time should only be block level offset
    virtual void send_event_time(uint32_t, unsigned int)
    {
    };
    virtual void send_sysex_time(const uint8_t *, size_t, size_t, unsigned int)
    {
    };

    void sysex_reset(size_t port, unsigned int time);

protected:
#pragma warning(disable: 4820) // x bytes padding added after data member
    unsigned long _SampleRate;
    system_exclusive_table mSysexMap;
    filter_mode mode;
    bool _IsInitialized;
    bool reverb_chorus_disabled;
    char _Padding[2]; 
#pragma warning(default: 4820)

private:
    void send_event_filtered(uint32_t b);
    void send_sysex_filtered(const uint8_t * event, size_t size, size_t port);
    void send_event_time_filtered(uint32_t b, unsigned int time);
    void send_sysex_time_filtered(const uint8_t * event, size_t size, size_t port, unsigned int time);

    void sysex_send_gs(size_t port, uint8_t * data, size_t size, unsigned int time);
    void sysex_reset_sc(uint32_t port, unsigned int time);

    unsigned long uSamplesRemaining;

    unsigned uLoopMode;

    std::vector<midi_stream_event> mStream;

    unsigned long uStreamPosition;
    unsigned long uTimeCurrent;
    unsigned long uTimeEnd;

    unsigned long uStreamLoopStart;
    unsigned long uTimeLoopStart;
    unsigned long uStreamEnd;
};
