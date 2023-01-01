 
/** $VER: InputDecoder.cpp (2022.12.31) **/

#pragma warning(disable: 5045 26481 26485)

#include "InputDecoder.h"

volatile int _IsRunning = 0;

critical_section _Lock;
volatile unsigned int _CurrentSampleRate;

static size_t DecodeShiftJISChar(const uint8_t * data, size_t size)
{
    if (size == 0)
        return 0;

    if (size > 2)
        size = 2;

    if (data[0] < 0x80)
        return (size_t)((data[0] > 0) ? 1 : 0);

    if (size >= 1)
    {
        // TODO: definitely weak
        if (unsigned(data[0] - 0xA1) < 0x3F)
            return 1;
    }

    if (size >= 2)
    {
        // TODO: probably very weak
        if ((unsigned(data[0] - 0x81) < 0x1F || unsigned(data[0] - 0xE0) < 0x10) && unsigned(data[1] - 0x40) < 0xBD)
            return 2;
    }

    return 0;
}

bool IsValidShiftJIS(const char * data, size_t size)
{
    for (size_t i = 0; (i < size) && (data[i] != 0);)
    {
        size_t n = ::DecodeShiftJISChar((const uint8_t *)data + i, size - i);

        if (n == 0)
            return false;

        i += n;

        if (i > size)
            return false;
    }

    return true;
}

/*
#define XMIDI_CONTROLLER_FOR_LOOP 0x74 // For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK 0x75 // Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION 110 // Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION 111 // Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END XMIDI_CONTROLLER_NEXT_BREAK
*/

static input_factory_t<input_midi> InputDecoderFactory;
