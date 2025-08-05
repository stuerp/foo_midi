// クラスの名前を変更してABIの衝突を回避する
#define BW_MidiSequencer EmuDeMidiMidiSequencer
// MIDIシーケンサークラスの実装を含める
#include "sequencer/midi_sequencer_impl.hpp"

#include "CSMFPlay.hpp"

namespace dsa
{

/****************************************************
 *           リアルタイムMIDI呼び出しプロキシ            *
 ****************************************************/

CMIDIModule &getModule(void *userdata, uint8_t channel)
{
    CSMFPlay *c = reinterpret_cast<CSMFPlay *>(userdata);
    return c->m_module[(channel * 2) % c->m_mods];
}

CMIDIModule &getModule2(void *userdata, uint8_t channel)
{
    CSMFPlay *c = reinterpret_cast<CSMFPlay *>(userdata);
    return c->m_module[(channel * 2 + 1) % c->m_mods];
}

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendNoteOn(channel, note, velocity);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendNoteOn(channel, note, velocity);
    }
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendNoteOff(channel, note, 0);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendNoteOff(channel, note, 0);
    }
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    // サポートされていません
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendChannelPressure(channel, atVal);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendChannelPressure(channel, atVal);
    }
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendControlChange(channel, type, value);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendControlChange(channel, type, value);
    }
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendProgramChange(channel, patch);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendProgramChange(channel, patch);
    }
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    CMIDIModule &m = getModule(userdata, channel);
    m.SendPitchBend(channel, msb, lsb);
    if(!m.IsDrum(channel))
    {
        CMIDIModule &m2 = getModule2(userdata, channel);
        m2.SendPitchBend(channel, msb, lsb);
    }
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    // サポートされていません
}


/* NonStandard calls */
static void rtDeviceSwitch(void *userdata, size_t track, const char *data, size_t length)
{
    // サポートされていません
}

static size_t rtCurrentDevice(void *userdata, size_t track)
{
    // サポートされていません
    return 0;
}
/* NonStandard calls End */


void playSynth(void *userdata, uint8_t *stream, size_t length)
{
    CSMFPlay *c = reinterpret_cast<CSMFPlay *>(userdata);
    DWORD len = static_cast<DWORD>(length / 8);
    int *buf = reinterpret_cast<int*>(stream);
    INT32 b[2];

    for(DWORD q = 0; q < len; q++)
    {
        buf[0] = buf[1] = 0;
        for(int i = 0; i < c->m_mods; i++)
        {
            c->m_module[i].Render(b);
            buf[0] += b[0];
            buf[1] += b[1];
        }
        buf += 2;
    }
}

void playSynthS16(void *userdata, uint8_t *stream, size_t length)
{
    CSMFPlay *c = reinterpret_cast<CSMFPlay *>(userdata);
    DWORD len = static_cast<DWORD>(length / 4);
    short *buf = reinterpret_cast<short*>(stream);
    INT32 b[2];

    for(DWORD q = 0; q < len; q++)
    {
        buf[0] = buf[1] = 0;
        for(int i = 0; i < c->m_mods; i++)
        {
            c->m_module[i].Render(b);
            buf[0] += (short)b[0];
            buf[1] += (short)b[1];
        }
        buf += 2;
    }
}

void playSynthF32(void *userdata, uint8_t *stream, size_t length)
{
    CSMFPlay *c = reinterpret_cast<CSMFPlay *>(userdata);
    DWORD len = static_cast<DWORD>(length / 8);
    float *buf = reinterpret_cast<float*>(stream);
    INT32 b[2];

    for(DWORD q = 0; q < len; q++)
    {
        buf[0] = buf[1] = 0;
        for(int i = 0; i < c->m_mods; i++)
        {
            c->m_module[i].Render(b);
            buf[0] += (float)b[0] / 0x7fff;
            buf[1] += (float)b[1] / 0x7fff;
        }
        buf += 2;
    }
}


void CSMFPlay::initSequencerInterface()
{
    m_sequencer = new MidiSequencer;
    BW_MidiRtInterface *seq = new BW_MidiRtInterface;
    if(m_sequencerInterface)
        delete m_sequencerInterface;
    m_sequencerInterface = seq;

    std::memset(seq, 0, sizeof(BW_MidiRtInterface));

    /* MIDIリアルタイムコール */
    seq->rtUserData = this;
    seq->rt_noteOn  = rtNoteOn;
    seq->rt_noteOff = rtNoteOff;
    seq->rt_noteAfterTouch = rtNoteAfterTouch;
    seq->rt_channelAfterTouch = rtChannelAfterTouch;
    seq->rt_controllerChange = rtControllerChange;
    seq->rt_patchChange = rtPatchChange;
    seq->rt_pitchBend = rtPitchBend;
    seq->rt_systemExclusive = rtSysEx;

    seq->onPcmRender = playSynthF32;
    seq->onPcmRender_userData = this;

    seq->pcmSampleRate = static_cast<uint32_t>(m_rate);
    seq->pcmFrameSize = 2 /*channels*/ * 4 /*size of one sample*/;

    /* 非標準コール */
    seq->rt_deviceSwitch = rtDeviceSwitch;
    seq->rt_currentDevice = rtCurrentDevice;

    m_sequencer->setInterface(seq);
}

double CSMFPlay::Tick(double s, double granularity)
{
    MidiSequencer &seqr = *m_sequencer;
    return seqr.Tick(s, granularity);
}

}//dsa
