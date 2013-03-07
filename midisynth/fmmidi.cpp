// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#include "fmmidi.hpp"

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

namespace midisynth{
namespace opn{
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

    // 対数変換テーブル。エンベロープジェネレータのディケイ以降で使う。
    namespace{
        #define LOG10_32767 4.5154366811416989472479934140484
        #define LOGTABLE_SIZE 4096
        #define LOGTABLE_FACTOR (LOGTABLE_SIZE / LOG10_32767)
        class log_table{
        public:
            log_table();
            uint_least16_t get(int x)const{ return data[x]; }
        private:
            uint_least16_t data[LOGTABLE_SIZE];
        }log_table;
        log_table::log_table()
        {
            for(int i = 0; i < LOGTABLE_SIZE; ++i){
                data[i] = std::pow(10, static_cast<double>(i) / LOGTABLE_FACTOR);
            }
        }
    }

    // レートテーブル。AR、DR、SR、RRの計算処理の高速化。
    namespace{
        struct fm_envelope_table{
            fm_envelope_table();
            uint_least32_t TL[128];
            uint_least32_t SL[16][128];
            double AR[64][128];
            double RR[64][128];
        }const fm_envelope_table;

        fm_envelope_table::fm_envelope_table()
        {
            for(int t = 0; t < 128; ++t){
                double fTL = 32767 * std::pow(10, t * -0.75 / 10);
                TL[t] = fTL;
                if(TL[t] == 0){
                    TL[t] = 1;
                }
                for(int s = 0; s < 16; ++s){
                    double x = fTL * std::pow(10, s * -3.0 / 10);
                    if(x <= 1){
                        SL[s][t] = 0;
                    }else{
                        SL[s][t] = 65536 * LOGTABLE_FACTOR * std::log10(x);
                    }
                }
            }
            for(int x = 0; x < 64; ++x){
                double attack_time = 15.3262 * std::pow(10, x * -0.75 / 10);
                double release_time = 211.84 * std::pow(10, x * -0.75 / 10);
                for(int t = 0; t < 128; ++t){
                    AR[x][t] = TL[t] / attack_time;
                    RR[x][t] = 65536 * LOGTABLE_FACTOR * 48.0 / 10 * TL[t] / 32767 / release_time;
                }
            }
        }
    }

    // エンベロープ生成器コンストラクタ。
    envelope_generator::envelope_generator(int AR_, int DR_, int SR_, int RR_, int SL, int TL_):
        AR(AR_), DR(DR_), SR(SR_), RR(RR_), TL(TL_),
        state(ATTACK), current(0), rate(1), hold(0), freeze(0)
    {
        if(AR >= 63) AR = 63;
        if(DR >= 63) DR = 63;
        if(SR >= 63) SR = 63;
        if(RR >= 63) RR = 63;
        assert(AR >= 0);
        assert(DR >= 0);
        assert(SR >= 0);
        assert(RR >= 0);
        assert(SL >= 0 && SL <= 15);
        assert(TL >= 0 && TL <= 127);

        fTL = fm_envelope_table.TL[TL];
        fSS = fSL = fm_envelope_table.SL[SL][TL];
        fAR = 0;
        fDR = 0;
        fSR = 0;
        fRR = 0;
        fOR = 0;
        fDRR = 0;
        fDSS = 0;
    }
    // 再生レートを設定する。
    inline void envelope_generator::set_rate(float rate)
    {
        this->rate = rate ? rate : 1;
        update_parameters();
    }
    // ホールド(ダンパー&ソステヌート)を設定する。
    void envelope_generator::set_hold(float hold)
    {
        if(this->hold > hold || state <= SASTAIN || current >= fSL){
            this->hold = hold;
            update_parameters();
        }
    }
    // フリーズを設定する。
    void envelope_generator::set_freeze(float freeze)
    {
        if(this->freeze > freeze || state <= SASTAIN || current >= fSL){
            this->freeze = freeze;
            update_parameters();
        }
    }
    // 各パラメータの更新。
    void envelope_generator::update_parameters()
    {
        double fAR = fm_envelope_table.AR[AR][TL] / rate;
        double fDR = fm_envelope_table.RR[DR][TL] / rate;
        double fSR = fm_envelope_table.RR[SR][TL] / rate;
        double fRR = fm_envelope_table.RR[RR][TL] / rate;

        if(fRR < 1){
            fRR = 1;
        }
        if(hold > 0){
            fRR = fSR * hold + fRR * (1 - hold);
        }
        if(freeze > 0){
            fDR *= (1 - freeze);
            fSR *= (1 - freeze);
            fRR *= (1 - freeze);
        }
        if(fAR < 1){
            fAR = 1;
        }
        this->fAR = static_cast<uint_least32_t>(fAR);
        this->fDR = static_cast<uint_least32_t>(fDR);
        this->fSR = static_cast<uint_least32_t>(fSR);
        this->fRR = static_cast<uint_least32_t>(fRR);
        this->fOR = static_cast<uint_least32_t>(fm_envelope_table.RR[63][0] / rate);
        this->fSS = std::max(this->fDR, fSL);
        this->fDRR = std::max(this->fDR, this->fRR);
        this->fDSS = std::max(this->fDRR, this->fSS);
    }
    // キーオフ。リリースに入る。
    void envelope_generator::key_off()
    {
        switch(state){
        case ATTACK:
            state = ATTACK_RELEASE;
            break;
        case DECAY:
            state = DECAY_RELEASE;
            break;
        case SASTAIN:
            state = RELEASE;
            break;
        default:
            break;
        }
    }
    // サウンドオフ。急速消音モードに入る。
    void envelope_generator::sound_off()
    {
        switch(state){
        case ATTACK:
        case ATTACK_RELEASE:
            if(current){
                current = 65536 * LOGTABLE_FACTOR * std::log10(static_cast<double>(current));
            }
            break;
        default:
            break;
        }
        state = SOUNDOFF;
    }
    // リリースからサウンドオフに移るレベル。リリースの長い音をいつまでも発音
    // してるとCPUパワーの無駄なので適当なところで切るため。減衰は対数なので
    // 音量が真にゼロになるには無限の時間を要する。実際には整数に切り捨てて
    // 処理しているので１未満になったら無音になるとはいえ…。
    // 不自然でない程度になるべく高い値の方がパフォーマンスが向上する。
    #define SOUNDOFF_LEVEL 1024
    // 次のサンプルを得る。
    int envelope_generator::get_next()
    {
        uint_least32_t current = this->current;
        switch(state){
        case ATTACK:
            if(current < fTL){
                return this->current = current + fAR;
            }
            this->current = 65536 * LOGTABLE_FACTOR * std::log10(static_cast<double>(fTL));
            state = DECAY;
            return fTL;
        case DECAY:
            if(current > fSS){
                this->current = current -= fDR;
                return log_table.get(current / 65536);
            }
            this->current = current = fSL;
            state = SASTAIN;
            return log_table.get(current / 65536);
        case SASTAIN:
            if(current > fSR){
                this->current = current -= fSR;
                int n = log_table.get(current / 65536);
                if(n > 1){
                    return n;
                }
            }
            state = FINISHED;
            return 0;
        case ATTACK_RELEASE:
            if(current < fTL){
                return this->current = current + fAR;
            }
            this->current = 65536 * LOGTABLE_FACTOR * std::log10(static_cast<double>(fTL));
            state = DECAY_RELEASE;
            return fTL;
        case DECAY_RELEASE:
            if(current > fDSS){
                this->current = current -= fDRR;
                return log_table.get(current / 65536);
            }
            this->current = current = fSL;
            state = RELEASE;
            return log_table.get(current / 65536);
        case RELEASE:
            if(current > fRR){
                this->current = current -= fRR;
                int n = log_table.get(current / 65536);
                if(n > SOUNDOFF_LEVEL){
                    return n;
                }
                state = SOUNDOFF;
                return n;
            }
            state = FINISHED;
            return 0;
        case SOUNDOFF:
            if(current > fOR){
                this->current = current -= fOR;
                int n = log_table.get(current / 65536);
                if(n > 1){
                    return n;
                }
            }
            state = FINISHED;
            return 0;
        default:
            return 0;
        }
    }

    namespace{
        // キースケーリングテーブル
        const int keyscale_table[4][128] = {
            {
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
            }, {
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
                 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
                 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
            }, {
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
                 2, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5,
                 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8,
                 8, 8, 8, 8, 8, 9, 9, 9,10,10,10,10,10,10,10,10,
                10,11,11,11,12,12,12,12,12,12,12,12,12,13,13,13,
                14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
            }, {
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 1, 1, 2, 2, 3, 4, 4, 4, 4, 4, 4, 4, 5,
                 5, 6, 6, 7, 8, 8, 8, 8, 8, 8, 8, 9, 9,10,10,11,
                12,12,12,12,12,12,12,13,13,14,14,15,16,16,16,16,
                16,16,16,17,17,18,18,19,20,20,20,20,20,20,20,21,
                21,22,22,23,24,24,24,24,24,24,24,25,25,26,26,27,
                28,28,28,28,28,28,28,29,29,30,30,31,31,31,31,31,
                31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31
            }
        };
        // ディチューンテーブル
        const float detune_table[4][128] = {
            { 0 },
            {
                0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
                0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
                0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
                0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053,
                0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053,
                0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.159, 0.159, 0.159, 0.159, 0.159,
                0.212, 0.212, 0.212, 0.212, 0.212, 0.212, 0.212, 0.212,
                0.212, 0.212, 0.212, 0.264, 0.264, 0.264, 0.264, 0.264,
                0.264, 0.264, 0.264, 0.317, 0.317, 0.317, 0.317, 0.370,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423
            }, {
                0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
                0.000, 0.000, 0.000, 0.000, 0.000, 0.053, 0.053, 0.053,
                0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053, 0.053,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.106 ,0.106, 0.159, 0.159, 0.159,
                0.212, 0.212, 0.212, 0.212, 0.212, 0.212, 0.212 ,0.212,
                0.212, 0.212, 0.212, 0.264, 0.264, 0.264, 0.264, 0.264,
                0.264, 0.264, 0.264, 0.317, 0.317, 0.317, 0.317, 0.370,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423, 0.423,
                0.423, 0.476, 0.476, 0.529, 0.582, 0.582, 0.582, 0.582,
                0.582, 0.582 ,0.582, 0.635, 0.635, 0.688, 0.688, 0.741,
                0.846, 0.846, 0.846, 0.846, 0.846, 0.846, 0.846 ,0.846,
                0.846, 0.846, 0.846, 0.846, 0.846, 0.846, 0.846, 0.846,
                0.846, 0.846, 0.846, 0.846, 0.846, 0.846, 0.846, 0.846
            }, {
                0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
                0.000, 0.000, 0.000, 0.000, 0.000, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106,
                0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.106, 0.159,
                0.159, 0.159, 0.159, 0.159, 0.212, 0.212, 0.212, 0.212,
                0.212, 0.212, 0.212, 0.212, 0.212, 0.212, 0.212, 0.264,
                0.264, 0.264, 0.264, 0.264, 0.264, 0.264, 0.264, 0.317,
                0.317, 0.317, 0.317, 0.370, 0.423, 0.423, 0.423, 0.423,
                0.423, 0.423, 0.423, 0.423, 0.423, 0.476, 0.476, 0.529,
                0.582, 0.582, 0.582, 0.582, 0.582, 0.582, 0.582, 0.635,
                0.635, 0.688, 0.688, 0.741, 0.846, 0.846, 0.846, 0.846,
                0.846, 0.846, 0.846, 0.899, 0.899, 1.005, 1.005, 1.058,
                1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164,
                1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164,
                1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164,
                1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164, 1.164
            }
        };
        // LFOテーブル
        const uint_least32_t ams_table[4] = {
            0,
            128 - 128 * std::pow(10, -1.44 / 10),
            128 - 128 * std::pow(10, -5.9 / 10),
            128 - 128 * std::pow(10, -11.8 / 10)
        };
    }

    // FMオペレータのコンストラクタ。
    fm_operator::fm_operator(int AR, int DR, int SR, int RR, int SL, int TL, int KS, int ML_, int DT_, int AMS_, int key):
        eg(AR * 2 + keyscale_table[KS][key],
           DR * 2 + keyscale_table[KS][key],
           SR * 2 + keyscale_table[KS][key],
           RR * 4 + keyscale_table[KS][key] + 2,
           SL,
           TL)
    {
        assert(AR >= 0 && AR <= 31);
        assert(DR >= 0 && DR <= 31);
        assert(SR >= 0 && SR <= 31);
        assert(RR >= 0 && RR <= 15);
        assert(SL >= 0);
        assert(TL >= 0);
        assert(KS >= 0 && KS <= 3);
        assert(ML_ >= 0 && ML_ <= 15);
        assert(DT_ >= 0 && DT_ <= 7);
        assert(AMS_ >= 0 && AMS_ <= 3);
        assert(key >= 0 && key <= 127);

        if(DT_ >= 4){
            DT = -detune_table[DT_ - 4][key];
        }else{
            DT = detune_table[DT_][key];
        }
        if(ML_ == 0){
            ML = 0.5;
        }else{
            ML = ML_;
        }

        ams_factor = ams_table[AMS_] / 2;
        ams_bias = 32768 - ams_factor * 256;
    }
    // 再生周波数設定。
    void fm_operator::set_freq_rate(float freq, float rate)
    {
        freq += DT;
        freq *= ML;
        swg.set_cycle(rate / freq);
        eg.set_rate(rate);
    }
    // 次のサンプルを得る。
    inline int fm_operator::get_next()
    {
        return static_cast<int_least32_t>(swg.get_next()) * eg.get_next() >> 15;
    }
    inline int fm_operator::get_next(int modulate)
    {
        return static_cast<int_least32_t>(swg.get_next(modulate)) * eg.get_next() >> 15;
    }
    inline int fm_operator::get_next(int ams, int modulate)
    {
        return (static_cast<int_least32_t>(swg.get_next(modulate)) * eg.get_next() >> 15) * (ams * ams_factor + ams_bias) >> 15;
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
    fm_sound_generator::fm_sound_generator(const FMPARAMETER& params, int note, float frequency_multiplier):
        op1(params.op1.AR, params.op1.DR, params.op1.SR, params.op1.RR, params.op1.SL, params.op1.TL, params.op1.KS, params.op1.ML, params.op1.DT, params.op1.AMS, note),
        op2(params.op2.AR, params.op2.DR, params.op2.SR, params.op2.RR, params.op2.SL, params.op2.TL, params.op2.KS, params.op2.ML, params.op2.DT, params.op2.AMS, note),
        op3(params.op3.AR, params.op3.DR, params.op3.SR, params.op3.RR, params.op3.SL, params.op3.TL, params.op3.KS, params.op3.ML, params.op3.DT, params.op3.AMS, note),
        op4(params.op4.AR, params.op4.DR, params.op4.SR, params.op4.RR, params.op4.SL, params.op4.TL, params.op4.KS, params.op4.ML, params.op4.DT, params.op4.AMS, note),
        ALG(params.ALG),
        freq(440 * std::pow(2.0, (note - 69) / 12.0)),
        freq_mul(frequency_multiplier),
        tremolo_depth(0),
        tremolo_freq(1),
        vibrato_depth(0),
        vibrato_freq(1),
        rate(0),
        feedback(0),
        damper(0),
        sostenute(0)
    {
        assert(ALG >= 0 && ALG <= 7);
        assert(params.LFO >= 0 && params.LFO <= 7);
        assert(params.FB >= 0 && params.FB <= 7);

        static const int feedbacks[8] = {
            31, 6, 5, 4, 3, 2, 1, 0
        };
        FB = feedbacks[params.FB];

        static const float ams_table[8] = {
            3.98, 5.56, 6.02, 6.37, 6.88, 9.63, 48.1, 72.2
        };
        ams_freq = ams_table[params.LFO];
        ams_enable = (params.op1.AMS + params.op2.AMS + params.op3.AMS + params.op4.AMS != 0);
    }
    // 再生レート設定。
    void fm_sound_generator::set_rate(float rate)
    {
        if(this->rate != rate){
            this->rate = rate;
            ams_lfo.set_cycle(rate / ams_freq);
            vibrato_lfo.set_cycle(rate / vibrato_freq);
            tremolo_lfo.set_cycle(rate / tremolo_freq);
            float f = freq * freq_mul;
            op1.set_freq_rate(f, rate);
            op2.set_freq_rate(f, rate);
            op3.set_freq_rate(f, rate);
            op4.set_freq_rate(f, rate);
        }
    }
    // 周波数倍率設定。
    void fm_sound_generator::set_frequency_multiplier(float value)
    {
        freq_mul = value;
        float f = freq * freq_mul;
        op1.set_freq_rate(f, rate);
        op2.set_freq_rate(f, rate);
        op3.set_freq_rate(f, rate);
        op4.set_freq_rate(f, rate);
    }
    // ダンパー効果設定。
    void fm_sound_generator::set_damper(int damper)
    {
        this->damper = damper;
        float value = 1.0 - (1.0 - damper / 127.0) * (1.0 - sostenute / 127.0);
        op1.set_hold(value);
        op2.set_hold(value);
        op3.set_hold(value);
        op4.set_hold(value);
    }
    // ソステヌート効果設定。
    void fm_sound_generator::set_sostenute(int sostenute)
    {
        this->sostenute = sostenute;
        float value = 1.0 - (1.0 - damper / 127.0) * (1.0 - sostenute / 127.0);
        op1.set_hold(value);
        op2.set_hold(value);
        op3.set_hold(value);
        op4.set_hold(value);
    }
    // フリーズ効果設定。
    void fm_sound_generator::set_freeze(int freeze)
    {
        float value = freeze / 127.0;
        op1.set_freeze(value);
        op2.set_freeze(value);
        op3.set_freeze(value);
        op4.set_freeze(value);
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
        op1.key_off();
        op2.key_off();
        op3.key_off();
        op4.key_off();
    }
    // サウンドオフ。
    void fm_sound_generator::sound_off()
    {
        op1.sound_off();
        op2.sound_off();
        op3.sound_off();
        op4.sound_off();
    }
    // 音の発生が終了したかどうかを返す。
    bool fm_sound_generator::is_finished()const
    {
        switch(ALG){
        case 0:
        case 1:
        case 2:
        case 3:
            return op4.is_finished();
        case 4:
            return op2.is_finished() && op4.is_finished();
        case 5:
        case 6:
            return op2.is_finished() && op3.is_finished() && op4.is_finished();
        case 7:
            return op1.is_finished() && op2.is_finished() && op3.is_finished() && op4.is_finished();
        default:
            assert(!"fm_sound_generator: invalid algorithm number");
            return true;
        }
    }
    // 次のサンプルを得る。
    int fm_sound_generator::get_next()
    {
        if(vibrato_depth){
            int x = static_cast<int_least32_t>(vibrato_lfo.get_next()) * vibrato_depth >> 15;
            int_least32_t modulation = vibrato_table.get(x);
            op1.add_modulation(modulation);
            op2.add_modulation(modulation);
            op3.add_modulation(modulation);
            op4.add_modulation(modulation);
        }
        int feedback = (this->feedback << 1) >> FB;
        int ret;
        if(ams_enable){
            int ams = ams_lfo.get_next() >> 7;
            switch(ALG){
            case 0:
                ret = op4(ams, op3(ams, op2(ams, this->feedback = op1(ams, feedback))));
                break;
            case 1:
                ret = op4(ams, op3(ams, op2(ams, 0) + (this->feedback = op1(ams, feedback))));
                break;
            case 2:
                ret = op4(ams, op3(ams, op2(ams, 0)) + (this->feedback = op1(ams, feedback)));
                break;
            case 3:
                ret = op4(ams, op3(ams, 0) + op2(ams, this->feedback = op1(ams, feedback)));
                break;
            case 4:
                ret = op4(ams, op3(ams, 0)) + op2(ams, this->feedback = op1(ams, feedback));
                break;
            case 5:
                this->feedback = feedback = op1(ams, feedback);
                ret = op4(ams, feedback) + op3(ams, feedback) + op2(ams, feedback);
                break;
            case 6:
                ret = op4(ams, 0) + op3(ams, 0) + op2(ams, this->feedback = op1(ams, feedback));
                break;
            case 7:
                ret = op4(ams, 0) + op3(ams, 0) + op2(ams, 0) + (this->feedback = op1(ams, feedback));
                break;
            default:
                assert(!"fm_sound_generator: invalid algorithm number");
                return 0;
            }
        }else{
            switch(ALG){
            case 0:
                ret = op4(op3(op2(this->feedback = op1(feedback))));
                break;
            case 1:
                ret = op4(op3(op2() + (this->feedback = op1(feedback))));
                break;
            case 2:
                ret = op4(op3(op2()) + (this->feedback = op1(feedback)));
                break;
            case 3:
                ret = op4(op3() + op2(this->feedback = op1(feedback)));
                break;
            case 4:
                ret = op4(op3()) + op2(this->feedback = op1(feedback));
                break;
            case 5:
                this->feedback = feedback = op1(feedback);
                ret = op4(feedback) + op3(feedback) + op2(feedback);
                break;
            case 6:
                ret = op4() + op3() + op2(this->feedback = op1(feedback));
                break;
            case 7:
                ret = op4() + op3() + op2() + (this->feedback = op1(feedback));
                break;
            default:
                assert(!"fm_sound_generator: invalid algorithm number");
                return 0;
            }
        }
        if(tremolo_depth){
            int_least32_t x = 4096 - (((static_cast<int_least32_t>(tremolo_lfo.get_next()) + 32768) * tremolo_depth) >> 11);
            ret = ret * x >> 12;
        }
        return ret;
    }

    // FMノートのコンストラクタ。
    fm_note::fm_note(const FMPARAMETER& params, int note, int velocity_, int panpot, int assign, float frequency_multiplier):
        note(assign, panpot),
        fm(params, note, frequency_multiplier),
        velocity(velocity_)
    {
        assert(velocity >= 1 && velocity <= 127);
        ++velocity;
    }
    // 波形出力。
    bool fm_note::synthesize(int_least32_t* buf, std::size_t samples, float rate, int_least32_t left, int_least32_t right)
    {
        left = (left * velocity) >> 7;
        right = (right * velocity) >> 7;
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
        // デフォルトの音色(sin波)
        static const struct FMPARAMETER param = {
            7, 0, 0,    // ALG FB LFO
            //AR DR SR RR SL  TL KS ML DT AMS
            { 31, 0, 0,15, 0,  0, 0, 0, 0, 0 },
            {  0, 0, 0,15, 0,127, 0, 0, 0, 0 },
            {  0, 0, 0,15, 0,127, 0, 0, 0, 0 },
            {  0, 0, 0,15, 0,127, 0, 0, 0, 0 }
        };
        drums.clear();
        programs.clear();
        programs[-1] = param;
    }
    // 音色パラメータの正当性検査
    namespace{
        bool is_valid_fmparameter(const FMPARAMETER& p)
        {
            return p.ALG >= 0 && p.ALG <= 7
                && p.FB >= 0 && p.FB <= 7
                && p.LFO >= 0 && p.LFO <= 7
                && p.op1.AR >= 0 && p.op1.AR <= 31
                && p.op1.DR >= 0 && p.op1.DR <= 31
                && p.op1.SR >= 0 && p.op1.SR <= 31
                && p.op1.RR >= 0 && p.op1.RR <= 15
                && p.op1.SL >= 0 && p.op1.SL <= 15
                && p.op1.TL >= 0 && p.op1.TL <= 127
                && p.op1.KS >= 0 && p.op1.KS <= 3
                && p.op1.ML >= 0 && p.op1.ML <= 15
                && p.op1.DT >= 0 && p.op1.DT <= 7
                && p.op1.AMS >= 0 && p.op1.AMS <= 3
                && p.op2.AR >= 0 && p.op2.AR <= 31
                && p.op2.DR >= 0 && p.op2.DR <= 31
                && p.op2.SR >= 0 && p.op2.SR <= 31
                && p.op2.RR >= 0 && p.op2.RR <= 15
                && p.op2.SL >= 0 && p.op2.SL <= 15
                && p.op2.TL >= 0 && p.op2.TL <= 127
                && p.op2.KS >= 0 && p.op2.KS <= 3
                && p.op2.ML >= 0 && p.op2.ML <= 15
                && p.op2.DT >= 0 && p.op2.DT <= 7
                && p.op2.AMS >= 0 && p.op2.AMS <= 3
                && p.op3.AR >= 0 && p.op3.AR <= 31
                && p.op3.DR >= 0 && p.op3.DR <= 31
                && p.op3.SR >= 0 && p.op3.SR <= 31
                && p.op3.RR >= 0 && p.op3.RR <= 15
                && p.op3.SL >= 0 && p.op3.SL <= 15
                && p.op3.TL >= 0 && p.op3.TL <= 127
                && p.op3.KS >= 0 && p.op3.KS <= 3
                && p.op3.ML >= 0 && p.op3.ML <= 15
                && p.op3.DT >= 0 && p.op3.DT <= 7
                && p.op3.AMS >= 0 && p.op3.AMS <= 3
                && p.op4.AR >= 0 && p.op4.AR <= 31
                && p.op4.DR >= 0 && p.op4.DR <= 31
                && p.op4.SR >= 0 && p.op4.SR <= 31
                && p.op4.RR >= 0 && p.op4.RR <= 15
                && p.op4.SL >= 0 && p.op4.SL <= 15
                && p.op4.TL >= 0 && p.op4.TL <= 127
                && p.op4.KS >= 0 && p.op4.KS <= 3
                && p.op4.ML >= 0 && p.op4.ML <= 15
                && p.op4.DT >= 0 && p.op4.DT <= 7
                && p.op4.AMS >= 0 && p.op4.AMS <= 3;
        }
        bool is_valid_drumparameter(const DRUMPARAMETER& p)
        {
            return is_valid_fmparameter(p)
                && p.key >= 0 && p.key <= 127
                && p.panpot >= 0 && p.panpot <= 16383;
        }
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
    bool fm_note_factory::set_program(int number, const FMPARAMETER& p)
    {
        if(is_valid_fmparameter(p)){
            programs[number] = p;
            return true;
        }else{
            return false;
        }
    }
    // ドラム音色パラメータをセット。
    bool fm_note_factory::set_drum_program(int number, const DRUMPARAMETER& p)
    {
        if(is_valid_drumparameter(p)){
            drums[number] = p;
            return true;
        }else{
            return false;
        }
    }
    // ノートオン。
    note* fm_note_factory::note_on(int_least32_t program, int note, int velocity, float frequency_multiplier)
    {
        bool drum = (program >> 14) == 120;
        if(drum){
            int n = (program & 0x3FFF) * 128 + note;
            struct DRUMPARAMETER* p;
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
            return new fm_note(*p, p->key, velocity, p->panpot, p->assign, 1);
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
