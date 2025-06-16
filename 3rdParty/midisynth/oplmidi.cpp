// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#include "oplmidi.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <utility>

#ifdef __BORLANDC__
#include <fastmath.h>
namespace std{
    using ::_fm_sin;
    using ::_fm_cos;
    using ::_fm_log10;
}
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "lanczos_resampler.h"

namespace midisynth{
namespace opl{
    namespace{
        struct init_resampler {
            init_resampler()
            {
                midi_lanczos_init();
            }
        };

        static init_resampler g_initializer;
    }

    // 正弦テーブル。正弦波ジェネレータ用。
    namespace{
        class sine_table{
        public:
            enum{ DIVISION = 4096 };
            sine_table();
            int_least16_t get(int n)const{ return data[n]; }
        private:
            int_least16_t data[DIVISION];
        }sine_table;

        sine_table::sine_table()
        {
            for(int i = 0; i < DIVISION; ++i){
                data[i] = static_cast<int_least16_t>(32767 * std::sin(i * 2 * M_PI / DIVISION));
            }
        }
    }
    // 正弦波生成器コンストラクタ。
    inline sine_wave_generator::sine_wave_generator():
        position(0), step(0)
    {
    }
    inline sine_wave_generator::sine_wave_generator(float cycle):
        position(0)
    {
        set_cycle(cycle);
    }
    // 正弦波の周期を変更する。
    void sine_wave_generator::set_cycle(float cycle)
    {
        if(cycle){
            step = static_cast<uint_least32_t>(sine_table::DIVISION * 32768.0 / cycle);
        }else{
            step = 0;
        }
    }
    // モジュレーションを加える。
    void sine_wave_generator::add_modulation(int_least32_t x)
    {
        position += static_cast<int_least32_t>(static_cast<int_least64_t>(step) * x >> 16);
    }
    // 次のサンプルを取り出す。
    inline int sine_wave_generator::get_next()
    {
        return sine_table.get((position += step) / 32768 % sine_table::DIVISION);
    }
    // 次のサンプルを取り出す(周波数変調付き)。
    inline int sine_wave_generator::get_next(int_least32_t modulation)
    {
        uint_least32_t m = modulation * sine_table::DIVISION / 65536;
        uint_least32_t p = ((position += step) / 32768 + m) % sine_table::DIVISION;
        return sine_table.get(p);
    }

	// ビブラートテーブル。
    namespace{
        class vibrato_table{
        public:
            enum{ DIVISION = 16384 };
            vibrato_table();
            int_least32_t get(int x)const{ return data[x + DIVISION / 2]; }
        private:
            int_least32_t data[DIVISION];
        }vibrato_table;

        vibrato_table::vibrato_table()
        {
            for(int i = 0; i < DIVISION; ++i){
                double x = (static_cast<double>(i) / DIVISION - 0.5) * 256.0 / 12.0;
                data[i] = static_cast<int_least32_t>((std::pow(2, x) - 1) * 65536.0);
            }
        }
    }
    
    // FM音源コンストラクタ。
    fm_sound_generator::fm_sound_generator(const FMPARAMETER& params, int note, int velocity, float frequency_multiplier):
        freq_mul(frequency_multiplier),
        tremolo_depth(0),
        tremolo_freq(1),
        vibrato_depth(0),
        vibrato_freq(1),
        rate(0)
    {
        double hertz;
        double phase_offset = 0.0125;
        bool set_vol_0;
        bool set_vol_1;
        bool set_vol_2;
        int x, y;

        int volume;

        velocity *= 100 * 100;

        volume = velocity>8725  ? std::log((double)velocity)*11.541561 + (0.5 - 104.22845) : 0;
        if (volume > 63) volume = 63;

        if(params.tone)
        {
            if(params.tone < 20)
                note += params.tone;
            else if(params.tone < 128)
                note = params.tone;
            else
                note -= params.tone-128;
        }

        buffer_ptr = 512;
        buffer_count = 0;

        blocks_on = params.key_on_ms * 50 / 256 + 64;
        blocks_off = params.key_off_ms * 50 / 256 + 64;

        resampler = midi_lanczos_resampler_create();

        fm_chip.Init(49716);
        fm_chip.WriteReg(0x004, 96);
        fm_chip.WriteReg(0x004, 128);
        fm_chip.WriteReg(0x105, 0);
        fm_chip.WriteReg(0x105, 1);
        fm_chip.WriteReg(0x001, 32);
        fm_chip.WriteReg(0x0BD, 0);
        switch (params.mode)
        {
        case FMPARAMETER::mode_single:
            set_vol_1 = true;
            set_vol_0 = !!(params.ops[0].feedconn & 1);
            fm_chip.WriteReg(0x020, params.ops[0].modulator_E862 & 0xFF);
            fm_chip.WriteReg(0x023, params.ops[0].carrier_E862 & 0xFF);
            fm_chip.WriteReg(0x060, (params.ops[0].modulator_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x063, (params.ops[0].carrier_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x080, (params.ops[0].modulator_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x083, (params.ops[0].carrier_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x0E0, (params.ops[0].modulator_E862 >> 24) & 0xFF);
            fm_chip.WriteReg(0x0E3, (params.ops[0].carrier_E862 >> 24) & 0xFF);
            x = params.ops[0].modulator_40;
            y = params.ops[0].carrier_40;
            fm_chip.WriteReg(0x040, set_vol_0 ? (x|63) - volume + volume*(x&63)/63 : x);
            fm_chip.WriteReg(0x043, set_vol_1 ? (y|63) - volume + volume*(y&63)/63 : y);
            fm_chip.WriteReg(0x0C0, params.ops[0].feedconn | 0x30);
            hertz = 172.00093 * exp(0.057762265 * (note + params.ops[0].finetune));
            if (hertz > 131071.0) hertz = 131071.0;
            freq[0] = 0x2000;
            while(hertz >= 1023.5) { hertz /= 2.0; freq[0] += 0x400; }
            freq[0] += (int)(hertz + 0.5);
            fm_chip.WriteReg(0x0A0, freq[0] & 0xFF);
            fm_chip.WriteReg(0x0B0, freq[0] >> 8);
            break;

        case FMPARAMETER::mode_fourop:
            phase_offset = 0.0;

            set_vol_0 = !!(params.ops[0].feedconn & 1);
            set_vol_1 = !(params.ops[0].feedconn & 1) && (params.ops[1].feedconn & 1);
            set_vol_2 = (params.ops[0].feedconn & 1) && (params.ops[1].feedconn & 1);

            fm_chip.WriteReg(0x104, 1);

            goto mode_fourop;

        case FMPARAMETER::mode_double:
            set_vol_0 = !!(params.ops[0].feedconn & 1);
            set_vol_1 = true;
            set_vol_2 = !!(params.ops[1].feedconn & 1);

mode_fourop:
            fm_chip.WriteReg(0x020, params.ops[0].modulator_E862 & 0xFF);
            fm_chip.WriteReg(0x023, params.ops[0].carrier_E862 & 0xFF);
            fm_chip.WriteReg(0x060, (params.ops[0].modulator_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x063, (params.ops[0].carrier_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x080, (params.ops[0].modulator_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x083, (params.ops[0].carrier_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x0E0, (params.ops[0].modulator_E862 >> 24) & 0xFF);
            fm_chip.WriteReg(0x0E3, (params.ops[0].carrier_E862 >> 24) & 0xFF);
            x = params.ops[0].modulator_40;
            y = params.ops[0].carrier_40;
            fm_chip.WriteReg(0x040, set_vol_0 ? (x|63) - volume + volume*(x&63)/63 : x);
            fm_chip.WriteReg(0x043, set_vol_1 ? (y|63) - volume + volume*(y&63)/63 : y);
            fm_chip.WriteReg(0x0C0, params.ops[0].feedconn | 0x30);
            fm_chip.WriteReg(0x028, params.ops[1].modulator_E862 & 0xFF);
            fm_chip.WriteReg(0x02B, params.ops[1].carrier_E862 & 0xFF);
            fm_chip.WriteReg(0x068, (params.ops[1].modulator_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x06B, (params.ops[1].carrier_E862 >> 8) & 0xFF);
            fm_chip.WriteReg(0x088, (params.ops[1].modulator_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x08B, (params.ops[1].carrier_E862 >> 16) & 0xFF);
            fm_chip.WriteReg(0x0E8, (params.ops[1].modulator_E862 >> 24) & 0xFF);
            fm_chip.WriteReg(0x0EB, (params.ops[1].carrier_E862 >> 24) & 0xFF);
            x = params.ops[1].modulator_40;
            y = params.ops[1].carrier_40;
            fm_chip.WriteReg(0x048, set_vol_2 ? (x|63) - volume + volume*(x&63)/63 : x);
            fm_chip.WriteReg(0x04B, (y|63) - volume + volume*(y&63)/63);
            fm_chip.WriteReg(0x0C8, params.ops[1].feedconn | 0x30);
            hertz = 172.00093 * exp(0.057762265 * (note + params.ops[0].finetune));
            if (hertz > 131071.0) hertz = 131071.0;
            freq[0] = 0x2000;
            while(hertz >= 1023.5) { hertz /= 2.0; freq[0] += 0x400; }
            freq[0] += (int)(hertz + 0.5);
            fm_chip.WriteReg(0x0A0, freq[0] & 0xFF);
            fm_chip.WriteReg(0x0B0, freq[0] >> 8);
            hertz = 172.00093 * exp(0.057762265 * (note + params.ops[1].finetune + phase_offset));
            if (hertz > 131071.0) hertz = 131071.0;
            freq[1] = 0x2000;
            while(hertz >= 1023.5) { hertz /= 2.0; freq[1] += 0x400; }
            freq[1] += (int)(hertz + 0.5);
            fm_chip.WriteReg(0x0A3, freq[1] & 0xFF);
            fm_chip.WriteReg(0x0B3, freq[1] >> 8);
            break;
        }
    }
    fm_sound_generator::~fm_sound_generator()
    {
        midi_lanczos_resampler_delete(resampler);
    }

    // 再生レート設定。
    void fm_sound_generator::set_rate(float rate)
    {
        if(this->rate != rate){
            this->rate = rate;
            vibrato_lfo.set_cycle(rate / vibrato_freq);
            tremolo_lfo.set_cycle(rate / tremolo_freq);
        }
    }
    // 周波数倍率設定。
    void fm_sound_generator::set_frequency_multiplier(float value)
    {
        freq_mul = value;
    }
    // ダンパー効果設定。
    void fm_sound_generator::set_damper(int damper)
    {
        (void)damper;
    }
    // ソステヌート効果設定。
    void fm_sound_generator::set_sostenute(int sostenute)
    {
        (void)sostenute;
    }
    // フリーズ効果設定。
    void fm_sound_generator::set_freeze(int freeze)
    {
        (void)freeze;
    }
    // トレモロ効果設定。
    void fm_sound_generator::set_tremolo(int depth, float frequency)
    {
        tremolo_depth = depth;
        tremolo_freq = frequency;
        tremolo_lfo.set_cycle(rate / frequency);
    }
    // ビブラート効果設定。
    void fm_sound_generator::set_vibrato(float depth, float frequency)
    {
        vibrato_depth = depth * (vibrato_table::DIVISION / 256.0);
        vibrato_freq = frequency;
        vibrato_lfo.set_cycle(rate / frequency);
    }
    // キーオフ。
    void fm_sound_generator::key_off()
    {
        freq[0] &= ~0x2000;
        freq[1] &= ~0x2000;
        fm_chip.WriteReg(0x0B0, freq[0] >> 8);
        fm_chip.WriteReg(0x0B3, freq[1] >> 8);
    }
    // サウンドオフ。
    void fm_sound_generator::sound_off()
    {
        key_off();
    }
    // 音の発生が終了したかどうかを返す。
    bool fm_sound_generator::is_finished()const
    {
        return buffer_count > ((freq[0] & 0x2000) ? blocks_on : blocks_off);
    }
    // 次のサンプルを得る。
    int fm_sound_generator::get_next()
    {
        int ret;
        double ratio = OPLRATE / rate * freq_mul;
        if(vibrato_depth){
            int x = static_cast<int_least32_t>(vibrato_lfo.get_next()) * vibrato_depth >> 15;
            int_least32_t modulation = vibrato_table.get(x);
            ratio += ratio * (double)modulation * (1.0 / 32768.0);
        }
        midi_lanczos_resampler_set_rate(resampler, ratio);
        while (!midi_lanczos_resampler_ready(resampler))
        {
            if (buffer_ptr == 512)
            {
                fm_chip.Generate(buffer, 256);
                buffer_ptr = 0;
                ++buffer_count;
            }
            midi_lanczos_resampler_write_sample(resampler, buffer[buffer_ptr] * 4);
            buffer_ptr += 2;
        }
        ret = midi_lanczos_resampler_get_sample(resampler);
        if (((unsigned)ret + 0x8000) & 0xFFFF0000) ret = (ret >> 31) ^ 0x7FFF;
        midi_lanczos_resampler_remove_sample(resampler);
        if(tremolo_depth){
            int_least32_t x = 4096 - (((static_cast<int_least32_t>(tremolo_lfo.get_next()) + 32768) * tremolo_depth) >> 11);
            ret = ret * x >> 12;
        }
        return ret;
    }

    // FMノートのコンストラクタ。
    fm_note::fm_note(const FMPARAMETER& params, int note, int velocity_, int panpot, int assign, float frequency_multiplier):
        ::midisynth::note(assign, panpot),
        fm(params, note, velocity_, frequency_multiplier)
    {
    }
    // 波形出力。
    bool fm_note::synthesize(int_least32_t* buf, std::size_t samples, float rate, int_least32_t left, int_least32_t right)
    {
        fm.set_rate(rate);
        for(std::size_t i = 0; i < samples; ++i){
            int_least32_t sample = fm.get_next();
            buf[i * 2 + 0] += (sample * left) >> 14;
            buf[i * 2 + 1] += (sample * right) >> 14;
        }
        return !fm.is_finished();
    }
    // ノートオフ。
    void fm_note::note_off(int)
    {
        fm.key_off();
    }
    // サウンドオフ。
    void fm_note::sound_off()
    {
        fm.sound_off();
    }
    // 周波数倍率設定。
    void fm_note::set_frequency_multiplier(float value)
    {
        fm.set_frequency_multiplier(value);
    }
    // トレモロ効果設定。
    void fm_note::set_tremolo(int depth, float freq)
    {
        fm.set_tremolo(depth, freq);
    }
    // ビブラート効果設定。
    void fm_note::set_vibrato(float depth, float freq)
    {
        fm.set_vibrato(depth, freq);
    }
    // ダンパー効果設定。
    void fm_note::set_damper(int value)
    {
        fm.set_damper(value);
    }
    // ソステヌート効果設定。
    void fm_note::set_sostenute(int value)
    {
        fm.set_sostenute(value);
    }
    // フリーズ効果設定。
    void fm_note::set_freeze(int value)
    {
        fm.set_freeze(value);
    }

    // FMノートファクトリ初期化。
    fm_note_factory::fm_note_factory()
    {
        clear();
    }
    // クリア。
    void fm_note_factory::clear()
    {
        static const struct FMPARAMETER param = {
            FMPARAMETER::mode_single, 0,
            1633, 1633,
            {
                { 0x0F4F201,0x0F7F201, 0x8F,0x06, 0x8,+0 },
                {         0,        0,    0,   0,   0, 0 }
            }
        };
        drums.clear();
        programs.clear();
        programs[-1] = param;
    }
    // 音色パラメータ取得。
    void fm_note_factory::get_program(int program, FMPARAMETER& p)
    {
        if(programs.find(program) != programs.end()){
            p = programs[program];
        }else if(programs.find(program & 0x3FFF) != programs.end()){
            p = programs[program & 0x3FFF];
        }else if(programs.find(program & 0x7F) != programs.end()){
            p = programs[program & 0x7F];
        }else{
            p = programs[-1];
        }
    }
    // 音色パラメータをセット。
    void fm_note_factory::set_program(int number, const FMPARAMETER& p)
    {
        programs[number] = p;
    }
    // ドラム音色パラメータをセット。
    void fm_note_factory::set_drum_program(int number, const FMPARAMETER& p)
    {
        drums[number] = p;
    }
    // ノートオン。
    note* fm_note_factory::note_on(int_least32_t program, int note, int velocity, float frequency_multiplier)
    {
        bool drum = (program >> 14) == 120;
        if(drum){
            int n = (program & 0x3FFF) * 128 + note;
            struct FMPARAMETER* p;
            if(drums.find(n) != drums.end()){
                p = &drums[n];
            }else if(drums.find(n & 0x3FFF) != drums.end()){
                p = &drums[n & 0x3FFF];
            }else if(drums.find(note) != drums.end()){
                p = &drums[note];
            }else if(drums.find(-1) != drums.end()){
                p = &drums[-1];
            }else{
                return NULL;
            }
            return new fm_note(*p, note, velocity, 8192, 0, 1);
        }else{
            struct FMPARAMETER* p;
            if(programs.find(program) != programs.end()){
                p = &programs[program];
            }else if(programs.find(program & 0x7F) != programs.end()){
                p = &programs[program & 0x7F];
            }else{
                p = &programs[-1];
            }
            return new fm_note(*p, note, velocity, 8192, 0, frequency_multiplier);
        }
    }
}
}
