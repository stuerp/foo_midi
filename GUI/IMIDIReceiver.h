
/** $VER: IMIDIReceiver.h (2023.01.15) P. Stuer **/

#pragma once

#include <stdint.h>

class IMIDIReceiver
{
public:
    IMIDIReceiver() = delete;

    IMIDIReceiver(const IMIDIReceiver &) = delete;
    IMIDIReceiver & operator=(const IMIDIReceiver &) = delete;
    IMIDIReceiver(IMIDIReceiver &&) = delete;
    IMIDIReceiver & operator=(IMIDIReceiver &&) = delete;

    virtual ~IMIDIReceiver() = delete;

    virtual void ProcessMessage(uint32_t message, uint32_t timestamp) = 0;
};
