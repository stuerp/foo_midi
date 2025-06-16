// Sample-based synthesizer
// Copyright(c)2013 Chris Moeller
#ifndef sampmidi_hpp
#define sampmidi_hpp

#include "midisynth.hpp"

namespace midisynth{
namespace samp{
    class envelope_generator{
    public:
        envelope_generator(float AR, float DR, float SR, float RR, float SL, float TL);
        void set_rate(float rate);
        void set_hold(float value);
        void set_freeze(float value);
        void key_off();
        void sound_off();
        bool is_finished()const{ return state == FINISHED; }
        float get_next();
    private:
        enum{ ATTACK, ATTACK_RELEASE, DECAY, DECAY_RELEASE, SASTAIN, RELEASE, SOUNDOFF, FINISHED }state;
        float AR, DR, SR, RR, TL, OR, SS, DRR, DSS;
        float current;
        float rate;
        float hold;
        float freeze;
        void update_parameters();
    };
	
	class sample{
	public:
		typedef enum {
			SD_BITS_16 = 0,
			SD_BITS_24,
			SD_BITS_32,
			SD_BITS_FLOAT
		} format;

		sample(void *sample_data, size_t size, format f);
		~sample();
		
		void add_ref();
		void release();
		
		float get_sample(size_t offset);
		
		size_t get_count() const;
			
	private:
		typedef float (*get_sample_func)(void *, size_t offset);
		
		void *sample_data;
		size_t size;
		size_t count;
		unsigned int reference_count;
		format fmt;
		get_sample_func getter;
	}

	class pcm_stream{
	public:
		typedef enum {
			loop_none = 0,
			loop_forward,
			loop_bidi
		} loop_mode;
		pcm_stream(sample * smp, size_t loop_start, size_t loop_end, loop_mode mode);
		~pcm_stream();
		
		float get_next();
		
		bool is_finished()const{ return state == FINISHED; }
		
	private:
        enum{ RUNNING=0, LOOPED, FINISHED }state;
		size_t position;
		size_t count;
		size_t loop_start;
		size_t loop_end;
		loop_mode mode;
	};
	
	class pcm_stream_enveloped{
	public:
		pcm_stream_enveloped(sample * smp, float AR, float DR, float SR, float RR, float SL, float TL, float KS, float ML, float DT, float AMS, int key);
        void set_freq_rate(float freq, float rate);
        void set_hold(float value){ eg.set_hold(value); }
        void set_freeze(float value){ eg.set_freeze(value); }
        void key_off(){ eg.key_off(); }
        void sound_off(){ eg.sound_off(); }
        bool is_finished()const{ return eg.is_finished() || stream.is_finished(); }
        float get_next();
        inline float operator()(){ return get_next(); }
	private:
		pcm_stream stream;
		envelope_generator eg;
	};

    // FMサウンドジェネレータ。
    class pcm_sound_generator{
    public:
        fm_sound_generator(sample *smp, int note, float frequency_multiplier);
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
