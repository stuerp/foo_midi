// ソフトウェアMIDIシンセサイザ。
// Copyright(c)2003-2004 yuno
#include "midisynth.hpp"

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
    // チャンネルコンストラクタ。
    channel::channel(note_factory* factory_, int bank):
        factory(factory_), default_bank(bank)
    {
        notes.reserve(16);
        reset_all_parameters();
    }
    // チャンネルデストラクタ。
    channel::~channel()
    {
        all_sound_off_immediately();
    }
    // 発音中の音を合成する。
    int channel::synthesize(int_least32_t* out, std::size_t samples, float rate, int_least32_t master_volume, int master_balance)
    {
        double volume = mute ? 0.0 : std::pow(static_cast<double>(master_volume) * this->volume * expression / (16383.0 * 16383.0 * 16383.0), 2) * 16383.0;
        int num_notes = 0;
        std::vector<NOTE>::iterator i = notes.begin();
        while(i != notes.end()){
            class note* note = i->note;
            uint_least32_t panpot = note->get_panpot();
            if(this->panpot <= 8192){
                panpot = panpot * this->panpot / 8192;
            }else{
                panpot = panpot * (16384 - this->panpot) / 8192 + (this->panpot - 8192) * 2;
            }
            if(master_balance <= 8192){
                panpot = panpot * master_balance / 8192;
            }else{
                panpot = panpot * (16384 - master_balance) / 8192 + (master_balance - 8192) * 2;
            }
            int_least32_t left = static_cast<int_least32_t>(volume * std::cos(std::max(0, (int)panpot - 1) * (M_PI / 2 / 16382)));
            int_least32_t right = static_cast<int_least32_t>(volume * std::sin(std::max(0, (int)panpot - 1) * (M_PI / 2 / 16382)));
            bool ret = note->synthesize(out, samples, rate, left, right);
            if(ret){
                ++i;
            }else{
                i = notes.erase(i);
                delete note;
            }
            ++num_notes;
        }
        return num_notes;
    }
    // すべてのパラメータを初期状態に戻す。
    void channel::reset_all_parameters()
    {
        program = default_bank * 128;
        bank = default_bank;
        panpot = 8192;
        volume = 12800;
        fine_tuning = 8192;
        coarse_tuning = 8192;
        tremolo_frequency = 3;
        vibrato_frequency = 3;
        master_frequency_multiplier = 1;
        mono = false;
        mute = false;
        system_mode = system_mode_default;
        reset_all_controller();
    }
    // パラメータを初期状態に戻す。
    void channel::reset_all_controller()
    {
        expression = 16383;
        channel_pressure(0);
        pitch_bend = 8192;
        pitch_bend_sensitivity = 256;
        update_frequency_multiplier();
        modulation_depth = 0;
        modulation_depth_range = 64;
        update_modulation();
        set_damper(0);
        set_sostenute(0);
        set_freeze(0);
        RPN = 0x3FFF;
        NRPN = 0x3FFF;
    }
    // すべての音をノートオフする。
    void channel::all_note_off()
    {
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            if(i->status == NOTE::NOTEON){
                i->status = NOTE::NOTEOFF;
                i->note->note_off(64);
            }
        }
    }
    // すべての音をサウンドオフする。
    void channel::all_sound_off()
    {
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            if(i->status != NOTE::SOUNDOFF){
                i->status = NOTE::SOUNDOFF;
                i->note->sound_off();
            }
        }
    }
    // 即時消音。
    void channel::all_sound_off_immediately()
    {
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            delete i->note;
        }
        notes.clear();
    }
    // ノートオン。音を出す。
    void channel::note_on(int note, int velocity)
    {
        assert(note >= 0 && note < NUM_NOTES);
        assert(velocity >= 0 && velocity <= 127);

        note_off(note, 64);
        if(velocity){
            if(mono){
                all_sound_off();
            }
            class note* p = factory->note_on(program, note, velocity, frequency_multiplier);
            if(p){
                int assign = p->get_assign();
                if(assign){
                    for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
                        if(i->note->get_assign() == assign){
                            i->note->sound_off();
                        }
                    }
                }
                if(freeze){
                    p->set_freeze(freeze);
                }
                if(damper){
                    p->set_damper(damper);
                }
                if(modulation_depth){
                    float depth = static_cast<double>(modulation_depth) * modulation_depth_range / (16383.0 * 128.0);
                    p->set_vibrato(depth, vibrato_frequency);
                }
                if(pressure){
                    p->set_tremolo(pressure, tremolo_frequency);
                }
                notes.push_back(NOTE(p, note));
            }
        }
    }
    // ノートオフ。音がリリースタイムに入る。
    void channel::note_off(int note, int velocity)
    {
        assert(note >= 0 && note < NUM_NOTES);
        assert(velocity >= 0 && velocity <= 127);
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            if(i->key == note && i->status == NOTE::NOTEON){
                i->status = NOTE::NOTEOFF;
                i->note->note_off(velocity);
            }
        }
    }
    // ポリフォニックキープレッシャ。
    void channel::polyphonic_key_pressure(int note, int value)
    {
        assert(note >= 0 && note < NUM_NOTES);
        assert(value >= 0 && value <= 127);
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            if(i->key == note && i->status == NOTE::NOTEON){
                i->note->set_tremolo(value, tremolo_frequency);
            }
        }
    }
    // チャンネルプレッシャ。
    void channel::channel_pressure(int value)
    {
        assert(value >= 0 && value <= 127);
        if(pressure != value){
            pressure = value;
            for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
                if(i->status == NOTE::NOTEON){
                    i->note->set_tremolo(value, tremolo_frequency);
                }
            }
        }
    }
    // コントロールチェンジ。
    void channel::control_change(int control, int value)
    {
        assert(value >= 0 && value <= 0x7F);
        switch(control){
        case 0x00:
            bank_select((bank & 0x7F) | (value << 7));
            break;
        case 0x01:
            set_modulation_depth((modulation_depth & 0x7F) | (value << 7));
            break;
        case 0x06:
            set_registered_parameter((get_registered_parameter() & 0x7F) | (value << 7));
            break;
        case 0x07:
            volume = (volume & 0x7F) | (value << 7);
            break;
        case 0x0A:
            panpot = (panpot & 0x7F) | (value << 7);
            break;
        case 0x0B:
            expression = (expression & 0x7F) | (value << 7);
            break;
        case 0x20:
            bank_select((bank & ~0x7F) | value);
            break;
        case 0x21:
            set_modulation_depth((modulation_depth & ~0x7F) | value);
            break;
        case 0x26:
            set_registered_parameter((get_registered_parameter() & ~0x7F) | value);
            break;
        case 0x27:
            volume = (volume & ~0x7F) | value;
            break;
        case 0x2A:
            panpot = (panpot & ~0x7F) | value;
            break;
        case 0x2B:
            expression = (expression & ~0x7F) | value;
            break;
        case 0x40:
            set_damper(value);
            break;
        case 0x42:
            set_sostenute(value);
            break;
        case 0x45:
            set_freeze(value);
            break;
        case 0x60:
            set_registered_parameter(std::max(0x3FFF, get_registered_parameter() + 1));
            break;
        case 0x61:
            set_registered_parameter(std::min(0, get_registered_parameter() - 1));
            break;
        case 0x62:
            set_NRPN((NRPN & ~0x7F) | value);
            break;
        case 0x63:
            set_NRPN((NRPN & 0x7F) | (value << 7));
            break;
        case 0x64:
            set_RPN((RPN & ~0x7F) | value);
            break;
        case 0x65:
            set_RPN((RPN & 0x7F) | (value << 7));
            break;
        case 0x78:
            all_sound_off();
            break;
        case 0x79:
            reset_all_controller();
            break;
        case 0x7B:
        case 0x7C:
        case 0x7D:
            all_note_off();
            break;
        case 0x7E:
            mono_mode_on();
            break;
        case 0x7F:
            poly_mode_on();
            break;
        }
    }
    // バンクセレクト
    void channel::bank_select(int value)
    {
        switch(system_mode){
        case system_mode_gm:
            break;
        case system_mode_gs:
            if(((bank & 0x3F80) == 0x3C00) == ((value & 0x3F80) == 0x3C00)){
                set_bank(value);
            }
            break;
        case system_mode_xg:
            if(default_bank == 0x3C00){
                set_bank(0x3C00 | (value & 0x7F));
            }else if((value & 0x3F80) == 0x3F80){
                set_bank(0x3C00 | (value & 0x7F));
            }else{
                set_bank(value);
            }
            break;
        default:
            if(default_bank == 0x3C00){
                set_bank(0x3C00 | (value & 0x7F));
            }else{
                set_bank(value);
            }
            break;
        }
    }
    // ダンパー効果。
    void channel::set_damper(int value)
    {
        if(damper != value){
            damper = value;
            for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
                i->note->set_damper(value);
            }
        }
    }
    // ソステヌート効果。
    void channel::set_sostenute(int value)
    {
        sostenute = value;
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            i->note->set_sostenute(value);
        }
    }
    // フリーズ効果。
    void channel::set_freeze(int value)
    {
        if(freeze != value){
            freeze = value;
            for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
                i->note->set_freeze(value);
            }
        }
    }
    // RPN取得。
    int channel::get_registered_parameter()
    {
        switch(RPN){
        case 0x0000:
            return pitch_bend_sensitivity;
        case 0x0001:
            return fine_tuning;
        case 0x0002:
            return coarse_tuning;
        case 0x0005:
            return modulation_depth_range;
        default:
            return 0;
        }
    }
    // RPN設定。
    void channel::set_registered_parameter(int value)
    {
        switch(RPN){
        case 0x0000:
            set_pitch_bend_sensitivity(value);
            break;
        case 0x0001:
            set_fine_tuning(value);
            break;
        case 0x0002:
            set_coarse_tuning(value);
            break;
        case 0x0005:
            set_modulation_depth_range(value);
            break;
        default:
            break;
        }
    }
    // 周波数倍率を再計算し更新する。
    void channel::update_frequency_multiplier()
    {
        float value = master_frequency_multiplier
                    * std::pow(2, (coarse_tuning - 8192) / (128.0 * 12.0)
                                + (fine_tuning - 8192) / (8192.0 * 12.0)
                                + static_cast<double>(pitch_bend - 8192) * pitch_bend_sensitivity / (8192.0 * 128.0 * 12.0));
        if(frequency_multiplier != value){
            frequency_multiplier = value;
            for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
                i->note->set_frequency_multiplier(value);
            }
        }
    }
    // モジュレーションデプス効果の更新。
    void channel::update_modulation()
    {
        float depth = static_cast<double>(modulation_depth) * modulation_depth_range / (16383.0 * 128.0);
        for(std::vector<NOTE>::iterator i = notes.begin(); i != notes.end(); ++i){
            i->note->set_vibrato(depth, vibrato_frequency);
        }
    }

    // シンセサイザコンストラクタ。
    synthesizer::synthesizer(note_factory* factory)
    {
        for(int i = 0; i < 16; ++i){
            channels[i].reset(new channel(factory, i == 9 ? 0x3C00 : 0x3C80));
        }
        reset_all_parameters();
    }
    // チャンネル取得。
    channel* synthesizer::get_channel(int ch)
    {
        assert(ch >= 0 && ch < NUM_CHANNELS);
        return channels[ch].get();
    }
    // 音を合成する。発音数を返す。
    int synthesizer::synthesize(int_least16_t* output, std::size_t samples, float rate)
    {
        std::size_t n = samples * 2;
        std::vector<int_least32_t> buf(n);
        int num_notes = synthesize_mixing(&buf[0], samples, rate);
        if(num_notes){
            for(std::size_t i = 0; i < n; ++i){
                int_least32_t x = buf[i];
                if(x < -32767){
                    output[i] = -32767;
                }else if(x > 32767){
                    output[i] = 32767;
                }else{
                    output[i] = static_cast<int_least16_t>(x);
                }
            }
        }else{
            std::memset(output, 0, sizeof(int_least16_t) * n);
        }
        return num_notes;
    }
    int synthesizer::synthesize_mixing(int_least32_t* output, std::size_t samples, float rate)
    {
        if(active_sensing == 0){
            all_sound_off();
            active_sensing = -1;
        }else if(active_sensing > 0){
            active_sensing = std::max(0.0f, active_sensing - samples / rate);
        }
        int_least32_t volume = static_cast<int_least32_t>(main_volume) * master_volume / 16384;
        int num_notes = 0;
        for(int i = 0; i < NUM_CHANNELS; ++i){
            num_notes += channels[i]->synthesize(output, samples, rate, volume, master_balance);
        }
        return num_notes;
    }
    // シンセサイザを完全にリセットする。
    void synthesizer::reset()
    {
        all_sound_off_immediately();
        reset_all_parameters();
    }
    // すべてのパラメータを初期状態に戻す。
    void synthesizer::reset_all_parameters()
    {
        active_sensing = -1;
        main_volume = 8192;
        master_volume = 16383;
        master_balance = 8192;
        master_fine_tuning = 8192;
        master_coarse_tuning = 8192;
        master_frequency_multiplier = 1;
        system_mode = system_mode_default;
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->reset_all_parameters();
        }
    }
    // パラメータを初期状態に戻す。
    void synthesizer::reset_all_controller()
    {
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->reset_all_controller();
        }
    }
    // オールノートオフ。すべての音をノートオフする。
    void synthesizer::all_note_off()
    {
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->all_note_off();
        }
    }
    // オールサウンドオフ。すべての音をサウンドオフする。
    void synthesizer::all_sound_off()
    {
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->all_sound_off();
        }
    }
    // 即時消音。
    void synthesizer::all_sound_off_immediately()
    {
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->all_sound_off_immediately();
        }
    }
    // システムエクスクルーシブメッセージの解釈実行。
    void synthesizer::sysex_message(const void* pvdata, std::size_t size)
    {
        const unsigned char* data = reinterpret_cast<const unsigned char*>(pvdata);
        if(size == 6 && std::memcmp(data, "\xF0\x7E\x7F\x09\x01\xF7", 6) == 0){
            /* GM system on */
            set_system_mode(system_mode_gm);
        }else if(size == 6 && std::memcmp(data, "\xF0\x7E\x7F\x09\x02\xF7", 6) == 0){
            /* GM system off */
            set_system_mode(system_mode_gm2);
        }else if(size == 6 && std::memcmp(data, "\xF0\x7E\x7F\x09\x03\xF7", 6) == 0){
            /* GM2 system on */
            set_system_mode(system_mode_gm2);
        }else if(size == 11 && std::memcmp(data, "\xF0\x41", 2) == 0 && std::memcmp(data + 3, "\x42\x12\x40\x00\x7F\x00\x41\xF7", 8) == 0){
            /* GS reset */
            set_system_mode(system_mode_gs);
        }else if(size == 9 && std::memcmp(data, "\xF0\x43", 2) == 0 && (data[2] & 0xF0) == 0x10 && std::memcmp(data + 3, "\x4C\x00\x00\x7E\x00\xF7", 6) == 0){
            /* XG system on */
            set_system_mode(system_mode_xg);
        }else if(size == 8 && std::memcmp(data, "\xF0\x7F\x7F\x04\x01", 5) == 0 && data[7] == 0xF7){
            /* master volume */
            set_master_volume((data[5] & 0x7F) | ((data[6] & 0x7F) << 7));
        }else if(size == 8 && std::memcmp(data, "\xF0\x7F\x7F\x04\x02", 5) == 0 && data[7] == 0xF7){
            /* master balance */
            set_master_balance((data[5] & 0x7F) | ((data[6] & 0x7F) << 7));
        }else if(size == 8 && std::memcmp(data, "\xF0\x7F\x7F\x04\x03", 5) == 0 && data[7] == 0xF7){
            /* master fine tuning */
            set_master_fine_tuning((data[5] & 0x7F) | ((data[6] & 0x7F) << 7));
        }else if(size == 8 && std::memcmp(data, "\xF0\x7F\x7F\x04\x04", 5) == 0 && data[7] == 0xF7){
            /* master coarse tuning */
            set_master_coarse_tuning((data[5] & 0x7F) | ((data[6] & 0x7F) << 7));
        }else if(size == 11 && std::memcmp(data, "\xF0\x41", 2) == 0 && (data[2] & 0xF0) == 0x10 && std::memcmp(data + 3, "\x42\x12\x40", 3) == 0 && (data[6] & 0xF0) == 0x10 && data[7] == 0x15 && data[10] == 0xF7){
            /* use for rhythm part */
            int channel = data[6] & 0x0F;
            int map = data[8];
            if(map == 0){
                channels[channel]->set_bank(0x3C80);
            }else{
                channels[channel]->set_bank(0x3C00);
            }
            channels[channel]->program_change(0);
        }
    }
    // MIDIイベントの解釈実行。
    void synthesizer::midi_event(int event, int param1, int param2)
    {
        switch(event & 0xF0){
        case 0x80:
            note_off(event & 0x0F, param1 & 0x7F, param2 & 0x7F);
            break;
        case 0x90:
            note_on(event & 0x0F, param1 & 0x7F, param2 & 0x7F);
            break;
        case 0xA0:
            polyphonic_key_pressure(event & 0x0F, param1 & 0x7F, param2 & 0x7F);
            break;
        case 0xB0:
            control_change(event & 0x0F, param1 & 0x7F, param2 & 0x7F);
            break;
        case 0xC0:
            program_change(event & 0x0F, param1 & 0x7F);
            break;
        case 0xD0:
            channel_pressure(event & 0x0F, param1 & 0x7F);
            break;
        case 0xE0:
            pitch_bend_change(event & 0x0F, ((param2 & 0x7F) << 7) | (param1 & 0x7F));
            break;
        case 0xFE:
            active_sensing = 0.33f;
            break;
        case 0xFF:
            all_sound_off();
            reset_all_parameters();
            break;
        default:
            break;
        }
    }
    // システムモードを変更する。
    void synthesizer::set_system_mode(system_mode_t mode)
    {
        all_sound_off();
        reset_all_parameters();
        system_mode = mode;
        for(int i = 0; i < NUM_CHANNELS; ++i){
            channels[i]->set_system_mode(mode);
        }
    }
    // マスターチューニングを再計算し更新する。
    void synthesizer::update_master_frequency_multiplier()
    {
        float value = std::pow(2, (master_coarse_tuning - 8192) / (128.0 * 100.0 * 12.0)
                                + (master_fine_tuning - 8192) / (8192.0 * 100.0 * 12.0));
        if(master_frequency_multiplier != value){
            master_frequency_multiplier = value;
            for(int i = 0; i < NUM_CHANNELS; ++i){
                channels[i]->set_master_frequency_multiplier(value);
            }
        }
    }
}