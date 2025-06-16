// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#ifndef midisynth_common_hpp
#define midisynth_common_hpp

#include <stdlib.h>
#include <stdint.h>

namespace midisynth{
	// コピー不可の基本クラス。
    class uncopyable{
    public:
        uncopyable(){}
    private:
        uncopyable(const uncopyable&){}
        void operator=(const uncopyable&){}
    };

    float midi_ct2hz_real(float cents);
    float midi_ct2hz(float cents);
};

#endif

