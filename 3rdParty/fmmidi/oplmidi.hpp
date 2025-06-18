// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#ifndef oplmidi_hpp
#define oplmidi_hpp

#include "midisynth.hpp"
#include "dbopl.hpp"

namespace midisynth{
namespace opl{
    // 正弦波生成器。
    // 振幅 32768 (-32767～32767) の正弦波を生成する。
    class sine_wave_generator{
    public:
        sine_wave_generator();
        sine_wave_generator(float cycle);
        void set_cycle(float cycle);
        void add_modulation(int_least32_t x);
        int get_next();
        int get_next(int_least32_t modulation);
    private:
        uint_least32_t position;
        uint_least32_t step;
    };

    struct FMVOICE{
        uint32_t modulator_E862, carrier_E862;
        uint8_t modulator_40, carrier_40;
        uint8_t feedconn;

        int8_t finetune;
    };

    struct FMPARAMETER{
        enum mode_t{
            mode_single = 0,
            mode_double,
            mode_fourop
        };

        mode_t mode;
        uint8_t tone;
        int key_on_ms;
        int key_off_ms;

        FMVOICE ops[2];
    };

    // FMサウンドジェネレータ。
    class fm_sound_generator{
    public:
        fm_sound_generator(const FMPARAMETER& params, int note, int velocity, float frequency_multiplier);
        ~fm_sound_generator();
        void set_rate(float rate);
        void set_frequency_multiplier(float value);
        void set_damper(int damper);
        void set_sostenute(int sostenute);
        void set_freeze(int freeze);
        void set_tremolo(int depth, float frequency);
        void set_vibrato(float depth, float frequency);
        void key_off();
        void sound_off();
        bool is_finished()const;
        int get_next();
    private:
        void *resampler;
        DBOPL0::Handler fm_chip;
        sine_wave_generator vibrato_lfo;
        sine_wave_generator tremolo_lfo;
        uint16_t freq[2];
        float freq_mul;
        int tremolo_depth;
        float tremolo_freq;
        int vibrato_depth;
        float vibrato_freq;
        float rate;
        int feedback[4];
        int buffer_ptr;
        int buffer_count;
        int blocks_on;
        int blocks_off;
        Bit32s buffer[512];
    };

	// FM音源ノート。
    class fm_note:public ::midisynth::note{
    public:
        fm_note(const FMPARAMETER& params, int note, int velocity, int panpot, int assign, float frequency_multiplier);
        virtual void release(){ delete this; }
        virtual bool synthesize(int_least32_t* buf, std::size_t samples, float rate, int_least32_t left, int_least32_t right);
        virtual void note_off(int velocity);
        virtual void sound_off();
        virtual void set_frequency_multiplier(float value);
        virtual void set_tremolo(int depth, float freq);
        virtual void set_vibrato(float depth, float freq);
        virtual void set_damper(int value);
        virtual void set_sostenute(int value);
        virtual void set_freeze(int value);
    public:
        fm_sound_generator fm;
    };

    // FM音源ノートファクトリ。
    class fm_note_factory:public ::midisynth::note_factory{
    public:
        fm_note_factory();
        void clear();
        void get_program(int number, FMPARAMETER& p);
        void set_program(int number, const FMPARAMETER& p);
        void set_drum_program(int number, const FMPARAMETER& p);
        virtual note* note_on(int_least32_t program, int note, int velocity, float frequency_multiplier);
    private:
        std::map<int, FMPARAMETER> programs;
        std::map<int, FMPARAMETER> drums;
    };
}
}

#endif
