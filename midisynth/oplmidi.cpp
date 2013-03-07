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

namespace midisynth{
namespace opl{
    // 正弦テーブル。正弦波ジェネレータ用。
    namespace{
        class wave_table{
        public:
            enum{ DIVISION = 4096 };
            wave_table();
            int_least16_t get(int w,int n)const{ return data[w][n]; }
        private:
            int_least16_t data[8][DIVISION];
        }wave_table;

        wave_table::wave_table()
        {
            for(int i = 0; i < DIVISION; ++i){
                data[0][i] = static_cast<int_least16_t>(32767 * std::sin(i * 2 * M_PI / DIVISION));
				data[1][i] = (i < (DIVISION / 2)) ? data[0][i] : 0;
				data[2][i] = data[0][i % (DIVISION / 2)];
				data[3][i] = (i & (DIVISION / 4)) ? 0 : data[0][i % (DIVISION / 4)];
				data[4][i] = (i < (DIVISION / 2)) ? data[0][i * 2] : 0;
				data[5][i] = (i < (DIVISION / 2)) ? data[0][(i * 2) % (DIVISION / 2)] : 0;
				data[6][i] = (i < (DIVISION / 2)) ? 32767 : -32767;
            }
			for (int i = 0; i < (DIVISION / 4); i++ ) {
				data[7][(DIVISION / 4) * 3 + i] = static_cast<int_least16_t>( 0.5 + ( pow(2.0, -1.0 + ( (DIVISION / 4) - 1 - i * 8) * ( 1.0 /(DIVISION / 4) ) ) ) * 4085 );
				data[7][(DIVISION / 4) * 3 - 1 - i ] = -data[7][(DIVISION / 4) * 3 + i];
				data[7][i] = 0;
				data[7][(DIVISION / 4) + i] = 0;
			}
        }
    }
    // 正弦波生成器コンストラクタ。
    inline wave_generator::wave_generator():
        wave(0), position(0), step(0)
    {
    }
    inline wave_generator::wave_generator(float cycle):
        wave(0), position(0)
    {
        set_cycle(cycle);
    }
	void wave_generator::set_wave(int wave)
	{
		if(wave < 8){
			this->wave = wave;
		}else{
			this->wave = 0;
		}
	}
    // 正弦波の周期を変更する。
    void wave_generator::set_cycle(float cycle)
    {
        if(cycle){
            step = static_cast<uint_least32_t>(wave_table::DIVISION * 32768.0 / cycle);
        }else{
            step = 0;
        }
    }
    // モジュレーションを加える。
    void wave_generator::add_modulation(int_least32_t x)
    {
        position += static_cast<int_least32_t>(static_cast<int_least64_t>(step) * x >> 16);
    }
    // 次のサンプルを取り出す。
    inline int wave_generator::get_next()
    {
        return wave_table.get(wave, (position += step) / 32768 % wave_table::DIVISION);
    }
    // 次のサンプルを取り出す(周波数変調付き)。
    inline int wave_generator::get_next(int_least32_t modulation)
    {
        uint_least32_t m = modulation * wave_table::DIVISION / 65536;
        uint_least32_t p = ((position += step) / 32768 + m) % wave_table::DIVISION;
        return wave_table.get(wave, p);
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

    // レートテーブル。AR、DR、RRの計算処理の高速化。
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
    envelope_generator::envelope_generator(int AR_, int DR_, int RR_, int SL, int TL_):
        AR(AR_), DR(DR_), RR(RR_), TL(TL_),
        state(ATTACK), current(0), rate(1), freeze(0)
    {
        if(AR >= 63) AR = 63;
        if(DR >= 63) DR = 63;
        if(RR >= 63) RR = 63;
        assert(AR >= 0);
        assert(DR >= 0);
        assert(RR >= 0);
        assert(SL >= 0 && SL <= 15);
        assert(TL >= 0 && TL <= 127);

        fTL = fm_envelope_table.TL[TL];
        fSS = fSL = fm_envelope_table.SL[SL][TL];
        fAR = 0;
        fDR = 0;
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
        double fRR = fm_envelope_table.RR[RR][TL] / rate;

        if(fRR < 1){
            fRR = 1;
        }
        if(freeze > 0){
            fDR *= (1 - freeze);
            fRR *= (1 - freeze);
        }
        if(fAR < 1){
            fAR = 1;
        }
        this->fAR = static_cast<uint_least32_t>(fAR);
        this->fDR = static_cast<uint_least32_t>(fDR);
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
        case SASTAIN:
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
    }

    // FMオペレータのコンストラクタ。
    fm_operator::fm_operator(int AR, int DR, int RR, int SL, int TL, int KS, int WS_, int key):
        eg(AR * 2 + keyscale_table[KS][key],
           DR * 2 + keyscale_table[KS][key],
           RR * 4 + keyscale_table[KS][key] + 2,
           SL,
           TL)
    {
        assert(AR >= 0 && AR <= 31);
        assert(DR >= 0 && DR <= 31);
        assert(RR >= 0 && RR <= 15);
        assert(SL >= 0);
        assert(TL >= 0);
        assert(KS >= 0 && KS <= 3);
		assert(WS_ >= 0 && WS_ <= 7);
        assert(key >= 0 && key <= 127);

		wg.set_wave(WS_);
    }
    // 再生周波数設定。
    void fm_operator::set_freq_rate(float freq, float rate)
    {
        wg.set_cycle(rate / freq);
        eg.set_rate(rate);
    }
    // 次のサンプルを得る。
    inline int fm_operator::get_next()
    {
        return static_cast<int_least32_t>(wg.get_next()) * eg.get_next() >> 15;
    }
    inline int fm_operator::get_next(int modulate)
    {
        return static_cast<int_least32_t>(wg.get_next(modulate)) * eg.get_next() >> 15;
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
		ops(params.ops),
        op1(params.op1.AR, params.op1.DR, params.op1.RR, params.op1.SL, params.op1.TL, params.op1.KS, params.op1.WS, note),
        op2(params.op2.AR, params.op2.DR, params.op2.RR, params.op2.SL, params.op2.TL, params.op2.KS, params.op2.WS, note),
        op3(params.op3.AR, params.op3.DR, params.op3.RR, params.op3.SL, params.op3.TL, params.op3.KS, params.op3.WS, note),
        op4(params.op4.AR, params.op4.DR, params.op4.RR, params.op4.SL, params.op4.TL, params.op4.KS, params.op4.WS, note),
        ALG(params.ALG),
        freq(440 * std::pow(2.0, (note - 69) / 12.0)),
        freq_mul(frequency_multiplier),
        tremolo_depth(0),
        tremolo_freq(1),
        vibrato_depth(0),
        vibrato_freq(1),
        rate(0),
        damper(0),
        sostenute(0)
    {
		assert(ops == 2 || ops == 4);
        assert(ALG >= 0 && ALG < ((ops == 2) ? 6 : 4));
        assert(params.FB[0] >= 0 && params.FB[0] <= 7);
        assert(params.FB[1] >= 0 && params.FB[1] <= 7);

        static const int feedbacks[8] = {
            31, 6, 5, 4, 3, 2, 1, 0
        };
        FB[0] = feedbacks[params.FB[0]];
        FB[1] = feedbacks[params.FB[1]];
		memset(feedback, 0, sizeof(feedback));
    }
    // 再生レート設定。
    void fm_sound_generator::set_rate(float rate)
    {
        if(this->rate != rate){
            this->rate = rate;
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
    }
    // ソステヌート効果設定。
    void fm_sound_generator::set_sostenute(int sostenute)
    {
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
		if (ops == 2){
			switch(ALG){
			case 0:
				return op2.is_finished();
			case 1:
				return op1.is_finished() && op2.is_finished();
			case 2:
				return op2.is_finished() && op4.is_finished();
			case 3:
				return op1.is_finished() && op2.is_finished() && op4.is_finished();
			case 4:
				return op2.is_finished() && op3.is_finished() && op4.is_finished();
			case 5:
				return op1.is_finished() && op2.is_finished() && op3.is_finished() && op4.is_finished();
	        default:
		        assert(!"fm_sound_generator: invalid algorithm number");
			    return true;
			}
		}else{
			switch(ALG){
			case 0:
				return op4.is_finished();
			case 1:
				return op1.is_finished() && op4.is_finished();
			case 2:
				return op2.is_finished() && op4.is_finished();
			case 3:
				return op1.is_finished() && op3.is_finished() && op4.is_finished();
	        default:
		        assert(!"fm_sound_generator: invalid algorithm number");
			    return true;
			}
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
        int feedback = ((this->feedback[0] + this->feedback[1]) << 1) >> FB[0];
		this->feedback[1] = this->feedback[0];
        int ret;
        if (ops == 2){
			this->feedback[0] = op1(feedback);
			if (ALG > 1){
				int feedback2 = ((this->feedback[2] + this->feedback[3]) << 1) >> FB[1];
				this->feedback[3] = this->feedback[2];
				this->feedback[2] = op3(feedback2);
			}
			switch(ALG){
			case 0:
				ret = op2(this->feedback[1]);
				break;
			case 1:
				ret = this->feedback[1] + op2();
			case 2:
				ret = op4(this->feedback[3]) + op2(this->feedback[1]);
				break;
			case 3:
				ret = op4(this->feedback[3]) + op2() + this->feedback[1];
				break;
			case 4:
				ret = op4() + this->feedback[3] + op2(this->feedback[1]);
				break;
			case 5:
				ret = op4() + this->feedback[3] + op2() + this->feedback[1];
				break;
            default:
                assert(!"fm_sound_generator: invalid algorithm number");
                return 0;
			}
		}else{
			this->feedback[0] = op1(feedback);
            switch(ALG){
            case 0:
                ret = op4(op3(op2(this->feedback[1])));
                break;
            case 1:
                ret = op4(op3(op2())) + this->feedback[1];
                break;
            case 2:
				ret = op4(op3()) + op2(this->feedback[1]);
                break;
            case 3:
                ret = op4() + op3(op2()) + this->feedback[1];
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
			2, 7, {0, 0},    // ops ALG FB
            //AR DR RR SL  TL KS AMS
            { 31, 0, 15, 0,  0, 0, 0 },
            {  0, 0, 15, 0,127, 0, 0 },
            {  0, 0, 15, 0,127, 0, 0 },
            {  0, 0, 15, 0,127, 0, 0 }
        };
        drums.clear();
        programs.clear();
        programs[-1] = param;
    }
    // 音色パラメータの正当性検査
    namespace{
        bool is_valid_fmparameter(const FMPARAMETER& p)
        {
            return (p.ops == 2 || p.ops == 4)
				&& ((p.ops == 2 && p.ALG >= 0 && p.ALG <= 5)
				   || (p.ops == 4 && p.ALG >= 0 && p.ALG <= 3))
                && p.FB[0] >= 0 && p.FB[0] <= 7
				&& p.FB[1] >= 0 && p.FB[1] <= 7
                && p.op1.AR >= 0 && p.op1.AR <= 31
                && p.op1.DR >= 0 && p.op1.DR <= 31
                && p.op1.RR >= 0 && p.op1.RR <= 15
                && p.op1.SL >= 0 && p.op1.SL <= 15
                && p.op1.TL >= 0 && p.op1.TL <= 127
                && p.op1.KS >= 0 && p.op1.KS <= 3
                && p.op2.AR >= 0 && p.op2.AR <= 31
                && p.op2.DR >= 0 && p.op2.DR <= 31
                && p.op2.RR >= 0 && p.op2.RR <= 15
                && p.op2.SL >= 0 && p.op2.SL <= 15
                && p.op2.TL >= 0 && p.op2.TL <= 127
                && p.op2.KS >= 0 && p.op2.KS <= 3
                && p.op3.AR >= 0 && p.op3.AR <= 31
                && p.op3.DR >= 0 && p.op3.DR <= 31
                && p.op3.RR >= 0 && p.op3.RR <= 15
                && p.op3.SL >= 0 && p.op3.SL <= 15
                && p.op3.TL >= 0 && p.op3.TL <= 127
                && p.op3.KS >= 0 && p.op3.KS <= 3
                && p.op4.AR >= 0 && p.op4.AR <= 31
                && p.op4.DR >= 0 && p.op4.DR <= 31
                && p.op4.RR >= 0 && p.op4.RR <= 15
                && p.op4.SL >= 0 && p.op4.SL <= 15
                && p.op4.TL >= 0 && p.op4.TL <= 127
                && p.op4.KS >= 0 && p.op4.KS <= 3;
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
