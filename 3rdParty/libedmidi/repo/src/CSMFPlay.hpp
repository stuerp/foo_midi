#ifndef __CSMFPLAY_HPP__
#define __CSMFPLAY_HPP__
#include <vector>
#include <string>

#include "emu_de_midi.h"
#include "CMIDIModule.hpp"

// クラスの名前を変更してABIの衝突を回避する
#define BW_MidiSequencer EmuDeMidiMidiSequencer
class BW_MidiSequencer;
typedef BW_MidiSequencer MidiSequencer;
typedef struct BW_MidiRtInterface BW_MidiRtInterface;

namespace dsa
{

class CSMFPlay
{
    friend CMIDIModule &getModule(void *userdata, uint8_t channel);
    friend CMIDIModule &getModule2(void *userdata, uint8_t channel);
    friend void playSynth(void *userdata, uint8_t *stream, size_t length);
    friend void playSynthS16(void *userdata, uint8_t *stream, size_t length);
    friend void playSynthF32(void *userdata, uint8_t *stream, size_t length);
    CMIDIModule m_module[16];

    int m_mods;
    int m_rate;

    int32_t m_outBuf[2048];

    std::string m_error;

    MidiSequencer *m_sequencer;
    BW_MidiRtInterface *m_sequencerInterface;
    void initSequencerInterface();

    double Tick(double s, double granularity);

public:
    CSMFPlay(DWORD rate, int mods = 4);
    ~CSMFPlay();

    bool Open(const char *filename);
    bool Load(const void *buf, int size);

    int Render(int *buf, size_t length);
    int RenderS16(short *buf, size_t length);
    int RenderF32(float *buf, size_t length);
    int RenderFormat(int sampleCount,
                     EDMIDI_UInt8 *left, EDMIDI_UInt8 *right,
                     const EDMIDI_AudioFormat *format);

    void Start(bool reset = true);
    void Reset();
    void Rewind();
    void Panic();

    void Seek(double seconds);
    double Tell();
    double Duration();
    double loopStart();
    double loopEnd();

    void SetLoop(bool enabled);
    void SetLoopsNumber(int loops);
    void setLoopHooksOnly(bool enabled);
    bool GetLoop();
    bool SeqEof();

    void setSongNum(int track);
    int getSongsCount();

    void setTempo(double tempo);
    double getTempo();

    int tracksCount();
    int setTrackEnabled(int track, bool en);
    int setChannelEnabled(int chan, bool en);

    void setTriggerHandler(EDMIDI_TriggerHandler handler, void *userData);

    const std::string &getMusicTitle();
    const std::string &getMusicCopyright();
    const std::vector<std::string> &getTrackTitles();
    size_t getMarkersCount();
    EdMidi_MarkerEntry getMarker(size_t index);

    void setDebugMessageHook(EDMIDI_DebugMessageHook debugMessageHook, void *userData);
    void adl_setLoopStartHook(EDMIDI_LoopPointHook loopStartHook, void *userData);
    void adl_setLoopEndHook(EDMIDI_LoopPointHook loopEndHook, void *userData);

    const std::string &getErrorString() const;
    void setErrorString(const std::string &err);
};

} // namespace dsa

#endif
