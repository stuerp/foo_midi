// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#pragma once

#include <Windows.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#define _USE_MATH_DEFINES

#include <stdint.h>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>

#include "midisynth_common.hpp"

namespace midisynth
{
    class channel;

    // システムモード列挙型。
    enum system_mode_t
    {
        system_mode_default,
        system_mode_gm,
        system_mode_gm2,
        system_mode_gs,
        system_mode_xg
    };

    // ノート。発音中の音。
    class note : uncopyable
    {
    public:
        note(int assign_, int panpot_) :assign(assign_), panpot(panpot_)
        {
        }

        virtual ~note()
        {
        }

        int get_assign()const
        {
            return assign;
        }

        int get_panpot()const
        {
            return panpot;
        }

        virtual bool synthesize(int_least32_t * buf, std::size_t samples, float rate, int_least32_t left, int_least32_t right) = 0;
        virtual void note_off(int velocity) = 0;
        virtual void sound_off() = 0;
        virtual void set_frequency_multiplier(float value) = 0;
        virtual void set_tremolo(int depth, float freq) = 0;
        virtual void set_vibrato(float depth, float freq) = 0;
        virtual void set_damper(int value) = 0;
        virtual void set_sostenute(int value) = 0;
        virtual void set_freeze(int value) = 0;

    private:
        int assign;
        int panpot;
    };

    class note_mixer :public note
    {
    public:
        note_mixer(int assign_, int panpot_) :note(assign_, panpot_)
        {
        }

        virtual ~note_mixer()
        {
        }

        virtual bool synthesize(int_least32_t * buf, std::size_t samples, float rate, int_least32_t left, int_least32_t right);
        virtual void note_off(int velocity);
        virtual void sound_off();
        virtual void set_frequency_multiplier(float value);
        virtual void set_tremolo(int depth, float freq);
        virtual void set_vibrato(float depth, float freq);
        virtual void set_damper(int value);
        virtual void set_sostenute(int value);
        virtual void set_freeze(int value);

        void add(class note * note_);

    private:
        struct NOTE
        {
            class note * note;

            NOTE(class note * note_) :note(note_)
            {
            }

            ~NOTE()
            {
                delete note;
            }
        };

        std::vector<NOTE> notes;
    };

    // ノートファクトリ。
    // ノートオンメッセージに対して適切なノートを作り出す。
    class note_factory : uncopyable
    {
    public:
        virtual note * note_on(int_least32_t program, int note, int velocity, float frequency_multiplier) = 0;

    protected:
        ~note_factory()
        {
        }
    };

    // MIDIチャンネル。
    class channel : uncopyable
    {
        enum
        {
            NUM_NOTES = 128
        };

    public:
        channel(note_factory * factory, int bank);
        ~channel();

        int synthesize(int_least32_t * out, std::size_t samples, float rate, int_least32_t master_volume, int master_balance);
        void reset_all_parameters();
        void reset_all_controller();
        void all_note_off();
        void all_sound_off();
        void all_sound_off_immediately();

        void note_off(int note, int velocity);
        void note_on(int note, int velocity);
        void polyphonic_key_pressure(int note, int value);

        void program_change(int value)
        {
            set_program(128 * bank + value);
        }

        void channel_pressure(int value);

        void pitch_bend_change(int value)
        {
            pitch_bend = value; update_frequency_multiplier();
        }

        void control_change(int control, int value);
        void bank_select(int value);

        void set_bank(int value)
        {
            bank = value;
        }

        void set_program(int value)
        {
            program = value;
        }

        void set_panpot(int value)
        {
            panpot = value;
        }

        void set_volume(int value)
        {
            volume = value;
        }

        void set_expression(int value)
        {
            expression = value;
        }

        void set_pitch_bend_sensitivity(int value)
        {
            pitch_bend_sensitivity = value; update_frequency_multiplier();
        }

        void set_modulation_depth(int value)
        {
            modulation_depth = value; update_modulation();
        }

        void set_modulation_depth_range(int value)
        {
            modulation_depth_range = value; update_modulation();
        }

        void set_damper(int value);
        void set_sostenute(int value);
        void set_freeze(int value);

        void set_fine_tuning(int value)
        {
            fine_tuning = value; update_frequency_multiplier();
        }

        void set_coarse_tuning(int value)
        {
            coarse_tuning = value; update_frequency_multiplier();
        }

        void set_RPN(int value)
        {
            RPN = value; NRPN = 0x3FFF;
        }

        void set_NRPN(int value)
        {
            NRPN = value; RPN = 0x3FFF;
        }

        void set_tremolo_frequency(float value)
        {
            tremolo_frequency = value;
        }

        void set_vibrato_frequency(float value)
        {
            vibrato_frequency = value;
        }

        void set_master_frequency_multiplier(float value)
        {
            master_frequency_multiplier = value; update_frequency_multiplier();
        }

        void set_mute(bool mute_)
        {
            mute = mute_;
        }
        void set_system_mode(system_mode_t mode);

        void mono_mode_on()
        {
            all_note_off(); mono = true;
        }

        void poly_mode_on()
        {
            all_note_off(); mono = false;
        }

        int get_program()const
        {
            return program;
        }

        int get_bank()const
        {
            return bank;
        }

        int get_panpot()const
        {
            return panpot;
        }

        int get_volume()const
        {
            return volume;
        }

        int get_expression()const
        {
            return expression;
        }
        int get_channel_pressure()const
        {
            return pressure;
        }
        int get_pitch_bend()const
        {
            return pitch_bend;
        }
        int get_pitch_bend_sensitivity()const
        {
            return pitch_bend_sensitivity;
        }
        int get_modulation_depth()const
        {
            return modulation_depth;
        }
        int get_modulation_depth_range()const
        {
            return modulation_depth_range;
        }
        int get_damper()const
        {
            return damper;
        }
        int get_sostenute()const
        {
            return sostenute;
        }
        int get_freeze()const
        {
            return freeze;
        }
        int get_fine_tuning()const
        {
            return fine_tuning;
        }
        int get_coarse_tuning()const
        {
            return coarse_tuning;
        }
        int get_resonance()const
        {
            return resonance;
        }
        int get_brightness()const
        {
            return brightness;
        }
        int get_RPN()const
        {
            return RPN;
        }
        int get_NRPN()const
        {
            return NRPN;
        }
        float get_tremolo_frequency()const
        {
            return tremolo_frequency;
        }
        float get_vibrato_frequency()const
        {
            return vibrato_frequency;
        }
        bool get_mute()const
        {
            return mute;
        }
        bool get_mono_mode()const
        {
            return mono;
        }

    private:
        struct NOTE
        {
            class note * note;
            int key;
            enum STATUS
            {
                NOTEON, NOTEOFF, SOUNDOFF
            }status;
            NOTE(class note * p, int key_) :note(p), key(key_), status(NOTEON)
            {
            }
        };

        std::vector<NOTE> notes;
        note_factory * factory;

        int default_bank;
        int program;
        int bank;
        int panpot;
        int volume;
        int expression;
        int pressure;
        int pitch_bend;
        int pitch_bend_sensitivity;
        int modulation_depth;
        int modulation_depth_range;
        int damper;
        int sostenute;
        int freeze;
        int fine_tuning;
        int coarse_tuning;
        int resonance;
        int brightness;
        int RPN;
        int NRPN;
        bool mono;
        bool mute;
        float tremolo_frequency;
        float vibrato_frequency;
        float frequency_multiplier;
        float master_frequency_multiplier;
        system_mode_t system_mode;

        int get_registered_parameter();
        void set_registered_parameter(int value);
        void update_frequency_multiplier();
        void update_modulation();
    };

    // MIDIシンセサイザ。
    class synthesizer : uncopyable
    {
        enum
        {
            NUM_CHANNELS = 16
        };

    public:
        synthesizer(note_factory * factory);

        channel * get_channel(int ch);

        int synthesize(int_least16_t * output, std::size_t samples, float rate);
        int synthesize_mixing(int_least32_t * output, std::size_t samples, float rate);
        void reset();
        void reset_all_parameters();
        void reset_all_controller();
        void all_note_off();
        void all_sound_off();
        void all_sound_off_immediately();

        void note_on(int channel, int note, int velocity)
        {
            get_channel(channel)->note_on(note, velocity);
        }

        void note_off(int channel, int note, int velocity)
        {
            get_channel(channel)->note_off(note, velocity);
        }

        void polyphonic_key_pressure(int channel, int note, int value)
        {
            get_channel(channel)->polyphonic_key_pressure(note, value);
        }

        void control_change(int channel, int control, int value)
        {
            get_channel(channel)->control_change(control, value);
        }

        void program_change(int channel, int program)
        {
            get_channel(channel)->program_change(program);
        }

        void channel_pressure(int channel, int value)
        {
            get_channel(channel)->channel_pressure(value);
        }

        void pitch_bend_change(int channel, int value)
        {
            get_channel(channel)->pitch_bend_change(value);
        }

        void sysex_message(const void * data, std::size_t size);

        void midi_event(int command, int param1, int param2);

        void midi_event(uint_least32_t message)
        {
            midi_event(message & 0xFF, (message >> 8) & 0x7F, (message >> 16) & 0x7F);
        }

        void set_main_volume(int value)
        {
            main_volume = value;
        }

        void set_master_volume(int value)
        {
            master_volume = value;
        }

        void set_master_balance(int value)
        {
            master_balance = value;
        }

        void set_master_fine_tuning(int value)
        {
            master_fine_tuning = value; update_master_frequency_multiplier();
        }

        void set_master_coarse_tuning(int value)
        {
            master_coarse_tuning = value; update_master_frequency_multiplier();
        }

        void set_system_mode(system_mode_t mode);

        int get_main_volume() const
        {
            return main_volume;
        }

        int get_master_volume() const
        {
            return master_volume;
        }

        int get_master_balance() const
        {
            return master_balance;
        }

        int get_master_fine_tuning() const
        {
            return master_fine_tuning;
        }

        int get_master_coarse_tuning() const
        {
            return master_coarse_tuning;
        }

        system_mode_t get_system_mode() const
        {
            return system_mode;
        }

    private:
        std::unique_ptr<channel> channels[NUM_CHANNELS];
        float active_sensing;
        int main_volume;
        int master_volume;
        int master_balance;
        int master_fine_tuning;
        int master_coarse_tuning;
        float master_frequency_multiplier;
        system_mode_t system_mode;
        void update_master_frequency_multiplier();
    };
}
