#ifdef _WIN32
#define NOMINMAX 1
#endif

#ifdef _WIN32
#   undef NO_OLDNAMES
#       include <stdint.h>
#   ifdef _MSC_VER
#       ifdef _WIN64
typedef __int64 ssize_t;
#       else
typedef __int32 ssize_t;
#       endif
#   else
#       ifdef _WIN64
typedef int64_t ssize_t;
#       else
typedef int32_t ssize_t;
#       endif
#   endif
#elif defined(__APPLE__) && defined(__POWERPC__)
#include <stdint.h>
typedef int32_t ssize_t;
#endif

#include <cstdio>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include <algorithm>
#include "CSMFPlay.hpp"
#include "COpllDevice.hpp"
#include "CSccDevice.hpp"

#include "sequencer/midi_sequencer.hpp"

#if defined (_MSC_VER)
#   if defined (_DEBUG)
#       define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#   endif
#endif

/*
 * Workaround for some compilers are has no those macros in their headers!
 */
#ifndef INT8_MIN
#define INT8_MIN    (-0x7f - 1)
#endif
#ifndef INT16_MIN
#define INT16_MIN   (-0x7fff - 1)
#endif
#ifndef INT32_MIN
#define INT32_MIN   (-0x7fffffff - 1)
#endif
#ifndef INT8_MAX
#define INT8_MAX    0x7f
#endif
#ifndef INT16_MAX
#define INT16_MAX   0x7fff
#endif
#ifndef INT32_MAX
#define INT32_MAX   0x7fffffff
#endif


using namespace dsa;

CSMFPlay::CSMFPlay(DWORD rate, int mods)
{
    m_sequencer = NULL;
    m_sequencerInterface = NULL;
    m_rate = rate;
    m_mods = mods;
    for(int i = 0; i < m_mods; i++)
    {
        if(i & 1)
            m_module[i].AttachDevice(new CSccDevice(rate, 2));
        else
            m_module[i].AttachDevice(new COpllDevice(rate, 2));
    }

    initSequencerInterface();
}

CSMFPlay::~CSMFPlay()
{
    for(int i = 0; i < m_mods; i++)
        delete m_module[i].DetachDevice();
    if(m_sequencer)
        delete m_sequencer;
    if(m_sequencerInterface)
        delete m_sequencerInterface;
}

bool CSMFPlay::Load(const void *buf, int size)
{
    bool ret = m_sequencer->loadMIDI(buf, size);
    Reset();
    return ret;
}

bool CSMFPlay::Open(const char *filename)
{
    bool ret = m_sequencer->loadMIDI(filename);
    Reset();
    return ret;
}

void CSMFPlay::Start(bool reset)
{
    if(reset)
        Reset();
    m_sequencer->rewind();
}

void CSMFPlay::Reset()
{
    for(int i = 0; i < m_mods; i++)
        m_module[i].Reset();
}

void CSMFPlay::Rewind()
{
    m_sequencer->rewind();
}

void CSMFPlay::Panic()
{
    for(int i = 0; i < m_mods; i++)
        m_module[i].SendPanic();
}

void CSMFPlay::Seek(double seconds)
{
    m_sequencer->seek(seconds, 1.0);
}

double CSMFPlay::Tell()
{
    return m_sequencer->tell();
}

double CSMFPlay::Duration()
{
    return m_sequencer->timeLength();
}

double CSMFPlay::loopStart()
{
    return m_sequencer->getLoopStart();
}

double CSMFPlay::loopEnd()
{
    return m_sequencer->getLoopEnd();
}

void CSMFPlay::SetLoop(bool enabled)
{
    m_sequencer->setLoopEnabled(enabled);
}

void CSMFPlay::SetLoopsNumber(int loops)
{
    m_sequencer->setLoopsCount(loops);
}

void CSMFPlay::setLoopHooksOnly(bool enabled)
{
    m_sequencer->setLoopHooksOnly(enabled);
}

bool CSMFPlay::GetLoop()
{
    return m_sequencer->getLoopEnabled();
}

bool CSMFPlay::SeqEof()
{
    return m_sequencer->positionAtEnd();
}

void CSMFPlay::setSongNum(int track)
{
    m_sequencer->setSongNum(track);
}

int CSMFPlay::getSongsCount()
{
    return m_sequencer->getSongsCount();
}

void CSMFPlay::setTempo(double tempo)
{
    m_sequencer->setTempo(tempo);
}

double CSMFPlay::getTempo()
{
    return m_sequencer->getTempoMultiplier();
}

int CSMFPlay::tracksCount()
{
    return (int)m_sequencer->getTrackCount();
}

int CSMFPlay::setTrackEnabled(int track, bool en)
{
    return (int)m_sequencer->setTrackEnabled(track, en);
}

int CSMFPlay::setChannelEnabled(int chan, bool en)
{
    return (int)m_sequencer->setChannelEnabled(chan, en);
}

void CSMFPlay::setTriggerHandler(EDMIDI_TriggerHandler handler, void *userData)
{
    m_sequencer->setTriggerHandler(handler, userData);
}

const std::string &CSMFPlay::getMusicTitle()
{
    return m_sequencer->getMusicTitle();
}

const std::string &CSMFPlay::getMusicCopyright()
{
    return m_sequencer->getMusicCopyright();
}

const std::vector<std::string> &CSMFPlay::getTrackTitles()
{
    return m_sequencer->getTrackTitles();
}

size_t CSMFPlay::getMarkersCount()
{
    return m_sequencer->getMarkers().size();
}

EdMidi_MarkerEntry CSMFPlay::getMarker(size_t index)
{
    struct EdMidi_MarkerEntry marker;
    const std::vector<MidiSequencer::MIDI_MarkerEntry> &markers = m_sequencer->getMarkers();
    if(index >= markers.size())
    {
        marker.label = "INVALID";
        marker.pos_time = 0.0;
        marker.pos_ticks = 0;
        return marker;
    }

    const MidiSequencer::MIDI_MarkerEntry &mk = markers[index];
    marker.label = mk.label.c_str();
    marker.pos_time = mk.pos_time;
    marker.pos_ticks = (unsigned long)mk.pos_ticks;

    return marker;
}

void CSMFPlay::setDebugMessageHook(EDMIDI_DebugMessageHook debugMessageHook, void *userData)
{
    m_sequencerInterface->onDebugMessage = debugMessageHook;
    m_sequencerInterface->onDebugMessage_userData = userData;
}

void CSMFPlay::adl_setLoopStartHook(EDMIDI_LoopPointHook loopStartHook, void *userData)
{
    m_sequencerInterface->onloopStart = loopStartHook;
    m_sequencerInterface->onloopStart_userData = userData;
}

void CSMFPlay::adl_setLoopEndHook(EDMIDI_LoopPointHook loopEndHook, void *userData)
{
    m_sequencerInterface->onloopEnd = loopEndHook;
    m_sequencerInterface->onloopEnd_userData = userData;
}

const std::string &CSMFPlay::getErrorString() const
{
    return m_error;
}

void CSMFPlay::setErrorString(const std::string &err)
{
    m_error = err;
}


namespace dsa
{
extern void playSynth(void *userdata, uint8_t *stream, size_t length);
extern void playSynthS16(void *userdata, uint8_t *stream, size_t length);
extern void playSynthF32(void *userdata, uint8_t *stream, size_t length);
}

int CSMFPlay::Render(int *buf, size_t length)
{
    if(m_sequencerInterface->onPcmRender != playSynth)
    {
        m_sequencerInterface->onPcmRender = playSynth;
        m_sequencerInterface->pcmFrameSize = 2 /*channels*/ * 4 /*size of one sample*/;
    }

    return m_sequencer->playStream(reinterpret_cast<uint8_t *>(buf), static_cast<size_t>(length * 8));
}

int CSMFPlay::RenderS16(short *buf, size_t length)
{
    if(m_sequencerInterface->onPcmRender != playSynthS16)
    {
        m_sequencerInterface->onPcmRender = playSynthS16;
        m_sequencerInterface->pcmFrameSize = 2 /*channels*/ * 2 /*size of one sample*/;
    }
    return m_sequencer->playStream(reinterpret_cast<uint8_t *>(buf), static_cast<size_t>(length * 4));
}

int CSMFPlay::RenderF32(float *buf, size_t length)
{
    if(m_sequencerInterface->onPcmRender != playSynthF32)
    {
        m_sequencerInterface->onPcmRender = playSynthF32;
        m_sequencerInterface->pcmFrameSize = 2 /*channels*/ * 4 /*size of one sample*/;
    }
    return m_sequencer->playStream(reinterpret_cast<uint8_t *>(buf), static_cast<size_t>(length * 8));
}



/*
  Sample conversions to various formats
*/
template <class Real>
inline Real adl_cvtReal(int32_t x)
{
    return static_cast<Real>(x) * (static_cast<Real>(1) / static_cast<Real>(INT16_MAX));
}

inline int32_t adl_cvtS16(int32_t x)
{
    x = (x < INT16_MIN) ? (INT16_MIN) : x;
    x = (x > INT16_MAX) ? (INT16_MAX) : x;
    return x;
}

inline int32_t adl_cvtS8(int32_t x)
{
    return adl_cvtS16(x) / 256;
}
inline int32_t adl_cvtS24(int32_t x)
{
    return adl_cvtS16(x) * 256;
}
inline int32_t adl_cvtS32(int32_t x)
{
    return adl_cvtS16(x) * 65536;
}
inline int32_t adl_cvtU16(int32_t x)
{
    return adl_cvtS16(x) - INT16_MIN;
}
inline int32_t adl_cvtU8(int32_t x)
{
    return (adl_cvtS16(x) / 256) - INT8_MIN;
}
inline int32_t adl_cvtU24(int32_t x)
{
    enum { int24_min = -(1 << 23) };
    return adl_cvtS24(x) - int24_min;
}
inline int32_t adl_cvtU32(int32_t x)
{
    // unsigned operation because overflow on signed integers is undefined
    return (uint32_t)adl_cvtS32(x) - (uint32_t)INT32_MIN;
}

template <class Dst>
static void CopySamplesRaw(EDMIDI_UInt8 *dstLeft, EDMIDI_UInt8 *dstRight, const int32_t *src,
                           size_t frameCount, unsigned sampleOffset)
{
    for(size_t i = 0; i < frameCount; ++i) {
        *(Dst *)(dstLeft + (i * sampleOffset)) = src[2 * i];
        *(Dst *)(dstRight + (i * sampleOffset)) = src[(2 * i) + 1];
    }
}

template <class Dst, class Ret>
static void CopySamplesTransformed(EDMIDI_UInt8 *dstLeft, EDMIDI_UInt8 *dstRight, const int32_t *src,
                                   size_t frameCount, unsigned sampleOffset,
                                   Ret(&transform)(int32_t))
{
    for(size_t i = 0; i < frameCount; ++i) {
        *(Dst *)(dstLeft + (i * sampleOffset)) = static_cast<Dst>(transform(src[2 * i]));
        *(Dst *)(dstRight + (i * sampleOffset)) = static_cast<Dst>(transform(src[(2 * i) + 1]));
    }
}

static int SendStereoAudio(int        samples_requested,
                           ssize_t    in_size,
                           int32_t   *_in,
                           ssize_t    out_pos,
                           EDMIDI_UInt8 *left,
                           EDMIDI_UInt8 *right,
                           const EDMIDI_AudioFormat *format)
{
    if(!in_size)
        return 0;
    size_t outputOffset = static_cast<size_t>(out_pos);
    size_t inSamples    = static_cast<size_t>(in_size * 2);
    size_t maxSamples   = static_cast<size_t>(samples_requested) - outputOffset;
    size_t toCopy       = std::min(maxSamples, inSamples);

    EDMIDI_SampleType sampleType = format->type;
    const unsigned containerSize = format->containerSize;
    const unsigned sampleOffset = format->sampleOffset;

    left  += (outputOffset / 2) * sampleOffset;
    right += (outputOffset / 2) * sampleOffset;

    typedef int32_t(&pfnConvert)(int32_t);
    typedef float(&ffnConvert)(int32_t);
    typedef double(&dfnConvert)(int32_t);

    switch(sampleType) {
    case EDMIDI_SampleType_S8:
    case EDMIDI_SampleType_U8:
    {
        pfnConvert cvt = (sampleType == EDMIDI_SampleType_S8) ? adl_cvtS8 : adl_cvtU8;
        switch(containerSize) {
        case sizeof(int8_t):
            CopySamplesTransformed<int8_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int16_t):
            CopySamplesTransformed<int16_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case EDMIDI_SampleType_S16:
    case EDMIDI_SampleType_U16:
    {
        pfnConvert cvt = (sampleType == EDMIDI_SampleType_S16) ? adl_cvtS16 : adl_cvtU16;
        switch(containerSize) {
        case sizeof(int16_t):
            CopySamplesTransformed<int16_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        case sizeof(int32_t):
            CopySamplesRaw<int32_t>(left, right, _in, toCopy / 2, sampleOffset);
            break;
        default:
            return -1;
        }
        break;
    }
    case EDMIDI_SampleType_S24:
    case EDMIDI_SampleType_U24:
    {
        pfnConvert cvt = (sampleType == EDMIDI_SampleType_S24) ? adl_cvtS24 : adl_cvtU24;
        switch(containerSize) {
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case EDMIDI_SampleType_S32:
    case EDMIDI_SampleType_U32:
    {
        pfnConvert cvt = (sampleType == EDMIDI_SampleType_S32) ? adl_cvtS32 : adl_cvtU32;
        switch(containerSize) {
        case sizeof(int32_t):
            CopySamplesTransformed<int32_t>(left, right, _in, toCopy / 2, sampleOffset, cvt);
            break;
        default:
            return -1;
        }
        break;
    }
    case EDMIDI_SampleType_F32:
    {
        if(containerSize != sizeof(float))
            return -1;
        ffnConvert cvt = adl_cvtReal<float>;
        CopySamplesTransformed<float>(left, right, _in, toCopy / 2, sampleOffset, cvt);
        break;
    }
    case EDMIDI_SampleType_F64:
    {
        if(containerSize != sizeof(double))
            return -1;
        dfnConvert cvt = adl_cvtReal<double>;
        CopySamplesTransformed<double>(left, right, _in, toCopy / 2, sampleOffset, cvt);
        break;
    }
    default:
        return -1;
    }

    return 0;
}


int CSMFPlay::RenderFormat(int sampleCount,
                           EDMIDI_UInt8 *out_left,
                           EDMIDI_UInt8 *out_right,
                           const EDMIDI_AudioFormat *format)
{
    size_t doRead = 1024;
    size_t doReadStereo = 512;
    int left = sampleCount;
    int gotten_len = 0;
    int generated = 0;
    int generatedSamples = 0;

    sampleCount -= sampleCount % 2; //Avoid even sample requests

    if(sampleCount < 0)
        return 0;

    if(m_sequencerInterface->onPcmRender != playSynth)
    {
        m_sequencerInterface->onPcmRender = playSynth;
        m_sequencerInterface->pcmFrameSize = 2 /*channels*/ * 4 /*size of one sample*/;
    }

    while(left > 0)
    {
        doRead = left > 1024 ? 1024 : left;
        doReadStereo = doRead / 2;
        generated = m_sequencer->playStream(reinterpret_cast<uint8_t *>(m_outBuf), static_cast<size_t>(doReadStereo * 8));
        assert(generated > 0);
        generatedSamples = generated / 4;

        /* Process it */
        if(SendStereoAudio(sampleCount, generatedSamples, m_outBuf, gotten_len, out_left, out_right, format) == -1)
            return 0;

        left -= generatedSamples;
        gotten_len += generatedSamples;
    }

    return gotten_len;
}
