// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#ifndef fmmidi_hpp
#define fmmidi_hpp

#include "midisynth.hpp"

namespace midisynth{
namespace opn{
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

    // エンベロープ生成器。
    // TL=0 のとき 0～32767 の値を生成する。
    class envelope_generator{
    public:
        envelope_generator(int AR, int DR, int SR, int RR, int SL, int TL);
        void set_rate(float rate);
        void set_hold(float value);
        void set_freeze(float value);
        void key_off();
        void sound_off();
        bool is_finished()const{ return state == FINISHED; }
        int get_next();
    private:
        enum{ ATTACK, ATTACK_RELEASE, DECAY, DECAY_RELEASE, SASTAIN, RELEASE, SOUNDOFF, FINISHED }state;
        int AR, DR, SR, RR, TL;
        uint_least32_t fAR, fDR, fSR, fRR, fSL, fTL, fOR, fSS, fDRR, fDSS;
        uint_least32_t current;
        float rate;
        float hold;
        float freeze;
        void update_parameters();
    };

    // FMオペレータ (モジュレータおよびキャリア)。
    class fm_operator{
    public:
        fm_operator(int AR, int DR, int SR, int RR, int SL, int TL, int KS, int ML, int DT, int AMS, int key);
        void set_freq_rate(float freq, float rate);
        void set_hold(float value){ eg.set_hold(value); }
        void set_freeze(float value){ eg.set_freeze(value); }
        void add_modulation(int_least32_t x){ swg.add_modulation(x); }
        void key_off(){ eg.key_off(); }
        void sound_off(){ eg.sound_off(); }
        bool is_finished()const{ return eg.is_finished(); }
        int get_next();
        int get_next(int modulate);
        int get_next(int lfo, int modulate);
        inline int operator()(){ return get_next(); }
        inline int operator()(int m){ return get_next(m); }
        inline int operator()(int lfo, int m){ return get_next(lfo, m); }
    private:
        sine_wave_generator swg;
        envelope_generator eg;
        float ML;
        float DT;
        int_least32_t ams_factor;
        int_least32_t ams_bias;
    };

    // FM音源パラメータ。
    struct FMPARAMETER{
        int ALG, FB, LFO;
        struct{
            int AR, DR, SR, RR, SL, TL, KS, ML, DT, AMS;
        }op1, op2, op3, op4;
    };
    // ドラムパラメータ。
    struct DRUMPARAMETER:FMPARAMETER{
        int key, panpot, assign;
    };

    // FMサウンドジェネレータ。
    class fm_sound_generator{
    public:
        fm_sound_generator(const FMPARAMETER& params, int note, float frequency_multiplier);
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
        fm_operator op1;
        fm_operator op2;
        fm_operator op3;
        fm_operator op4;
        sine_wave_generator ams_lfo;
        sine_wave_generator vibrato_lfo;
        sine_wave_generator tremolo_lfo;
        int ALG;
        int FB;
        float freq;
        float freq_mul;
        float ams_freq;
        bool ams_enable;
        int tremolo_depth;
        float tremolo_freq;
        int vibrato_depth;
        float vibrato_freq;
        float rate;
        int feedback;
        int damper;
        int sostenute;
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
        int velocity;
    };

    // FM音源ノートファクトリ。
    class fm_note_factory:public ::midisynth::note_factory{
    public:
        fm_note_factory();
        void clear();
        void get_program(int number, FMPARAMETER& p);
        bool set_program(int number, const FMPARAMETER& p);
        bool set_drum_program(int number, const DRUMPARAMETER& p);
        virtual note* note_on(int_least32_t program, int note, int velocity, float frequency_multiplier);
    private:
        std::map<int, FMPARAMETER> programs;
        std::map<int, DRUMPARAMETER> drums;
    };
}
}

#endif