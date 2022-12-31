
/** $VER: MIDIPlayer.cpp (2022.12.30) **/

#include "MIDIPlayer.h"

#pragma warning(disable: 5045)

MIDIPlayer::MIDIPlayer()
{
    _SamplesRemaining = 0;
    _SampleRate = 1000;
    _TimeCurrent = 0;
    _TimeEnd = 0;
    _LoopStartTime = 0;
    _IsInitialized = false;
}

bool MIDIPlayer::Load(const midi_container & midiContainer, unsigned subsong, unsigned loopMode, unsigned cleanFlags)
{
    assert(_Stream.size() == 0);

    midiContainer.serialize_as_stream(subsong, _Stream, _SysExMap, _LoopStart, _LoopEnd, cleanFlags);

    if (_Stream.size() == 0)
        return false;

    _StreamCurrent = 0;
    _TimeCurrent = 0;
    _TimeEnd = midiContainer.get_timestamp_end(subsong, true) + 1000;

    _LoopMode = loopMode;

    if (_LoopMode & loop_mode_enable)
    {
        _LoopStartTime = midiContainer.get_timestamp_loop_start(subsong, true);

        unsigned long LoopEndTime = midiContainer.get_timestamp_loop_end(subsong, true);

        if (_LoopStartTime != ~0UL || LoopEndTime != ~0UL)
            _LoopMode |= loop_mode_force;

        if (_LoopStartTime == ~0UL)
            _LoopStartTime = 0;

        if (LoopEndTime == ~0UL)
            LoopEndTime = _TimeEnd - 1000;

        if ((_LoopMode & loop_mode_force))
        {
            unsigned long i;
            uint8_t nullByte = 0;
            std::vector<uint8_t> note_on;

            note_on.resize(128 * 16, nullByte);

            ::memset(&note_on[0], 0, sizeof(note_on));

            for (i = 0; i < _Stream.size() && i < _LoopEnd; i++)
            {
                uint32_t ev = _Stream.at(i).m_event & 0x800000F0;

                if (ev == 0x90 || ev == 0x80)
                {
                    const unsigned long port = (_Stream.at(i).m_event & 0x7F000000) >> 24;
                    const unsigned long ch = _Stream.at(i).m_event & 0x0F;
                    const unsigned long note = (_Stream.at(i).m_event >> 8) & 0x7F;
                    const bool on = (ev == 0x90) && (_Stream.at(i).m_event & 0xFF0000);
                    const unsigned long bit = (unsigned long)(1 << port);

                    note_on.at(ch * 128 + note) = (uint8_t)((note_on.at(ch * 128 + note) & ~bit) | (bit * on));
                }
            }

            _Stream.resize(i);

            _TimeEnd = LoopEndTime - 1;

            if (_TimeEnd < _Stream.at(i - 1).m_timestamp)
                _TimeEnd = _Stream.at(i - 1).m_timestamp;

            for (unsigned long j = 0; j < 128 * 16; j++)
            {
                if (note_on.at(j))
                {
                    for (unsigned long k = 0; k < 8; k++)
                    {
                        if (note_on.at(j) & (1 << k))
                        {
                            _Stream.push_back(midi_stream_event(_TimeEnd, static_cast<uint32_t>((k << 24) + (j >> 7) + (j & 0x7F) * 0x100 + 0x90)));
                        }
                    }
                }
            }

            _TimeEnd = LoopEndTime;
        }
    }

    if (_SampleRate != 1000)
    {
        unsigned long SampleRate = static_cast<unsigned long>(_SampleRate);

        _SampleRate = 1000;

        setSampleRate(SampleRate);
    }

    return true;
}

unsigned long MIDIPlayer::Play(audio_sample * out, unsigned long count)
{
    assert(_Stream.size());

    if (!startup())
        return 0;

    unsigned long done = 0;

    const unsigned int needs_block_size = send_event_needs_time();
    unsigned int into_block = 0;

    // This should be a multiple of block size, and have leftover

    while (_SamplesRemaining && done < count)
    {
        unsigned long todo = _SamplesRemaining;

        if (todo > count - done)
            todo = count - done;

        if (needs_block_size && todo > needs_block_size)
            todo = needs_block_size;

        if (todo < needs_block_size)
        {
            _SamplesRemaining = 0;
            into_block = todo;
            break;
        }

        render(out + done * 2, todo);

        _SamplesRemaining -= todo;
        done += todo;
        _TimeCurrent += todo;
    }

    while (done < count)
    {
        unsigned long todo = _TimeEnd - _TimeCurrent;

        if (todo > count - done)
            todo = count - done;

        const unsigned long time_target = todo + _TimeCurrent;
        unsigned long stream_end = _StreamCurrent;

        while (stream_end < _Stream.size() && _Stream.at(stream_end).m_timestamp < time_target)
            stream_end++;

        if (stream_end > _StreamCurrent)
        {
            for (; _StreamCurrent < stream_end; _StreamCurrent++)
            {
                const midi_stream_event & me = _Stream.at(_StreamCurrent);

                unsigned long samples_todo = me.m_timestamp - _TimeCurrent - into_block;

                if (samples_todo)
                {
                    if (samples_todo > count - done)
                    {
                        _SamplesRemaining = samples_todo - (count - done);
                        samples_todo = count - done;
                    }

                    if (!needs_block_size && samples_todo)
                    {
                        render(out + done * 2, samples_todo);
                        done += samples_todo;
                        _TimeCurrent += samples_todo;
                    }

                    if (_SamplesRemaining)
                    {
                        _SamplesRemaining += into_block;
                        return done;
                    }
                }

                if (needs_block_size)
                {
                    into_block += samples_todo;

                    while (into_block >= needs_block_size)
                    {
                        render(out + done * 2, needs_block_size);

                        done += needs_block_size;
                        into_block -= needs_block_size;
                        _TimeCurrent += needs_block_size;
                    }
                    send_event_time_filtered(me.m_event, into_block);
                }
                else
                    send_event_filtered(me.m_event);
            }
        }

        if (done < count)
        {
            unsigned long samples_todo;

            if (_StreamCurrent < _Stream.size())
                samples_todo = _Stream.at(_StreamCurrent).m_timestamp;
            else
                samples_todo = _TimeEnd;

            samples_todo -= _TimeCurrent;

            if (needs_block_size)
                into_block = samples_todo;

            if (samples_todo > count - done)
                samples_todo = count - done;

            if (needs_block_size && samples_todo > needs_block_size)
                samples_todo = needs_block_size;

            if (samples_todo >= needs_block_size)
            {
                render(out + done * 2, samples_todo);

                done += samples_todo;
                _TimeCurrent += samples_todo;

                if (needs_block_size)
                    into_block -= samples_todo;
            }
        }

        if (!needs_block_size)
            _TimeCurrent = time_target;

        if (time_target >= _TimeEnd)
        {
            if (_StreamCurrent < _Stream.size())
            {
                for (; _StreamCurrent < _Stream.size(); _StreamCurrent++)
                {
                    if (needs_block_size)
                        send_event_time_filtered(_Stream.at(_StreamCurrent).m_event, into_block);
                    else
                        send_event_filtered(_Stream.at(_StreamCurrent).m_event);
                }
            }

            if ((_LoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
            {
                if (_LoopStart == ~0)
                {
                    _StreamCurrent = 0;
                    _TimeCurrent = 0;
                }
                else
                {
                    _StreamCurrent = _LoopStart;
                    _TimeCurrent = _LoopStartTime;
                }
            }
            else
                break;
        }
    }

    _SamplesRemaining = into_block;

    return done;
}

void MIDIPlayer::Seek(unsigned long sample)
{
    if (sample >= _TimeEnd)
    {
        if ((_LoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
        {
            while (sample >= _TimeEnd)
                sample -= _TimeEnd - _LoopStartTime;
        }
        else
        {
            sample = _TimeEnd;
        }
    }

    if (_TimeCurrent > sample)
    {
        _StreamCurrent = 0;

        if (!reset())
            shutdown();
    }

    if (!startup())
        return;

    _TimeCurrent = sample;

    std::vector<midi_stream_event> filler;

    unsigned long stream_start = _StreamCurrent;

    for (; _StreamCurrent < _Stream.size() && _Stream.at(_StreamCurrent).m_timestamp < _TimeCurrent; _StreamCurrent++)
        ;

    if (_StreamCurrent == _Stream.size())
        _SamplesRemaining = _TimeEnd - _TimeCurrent;
    else
        _SamplesRemaining = _Stream.at(_StreamCurrent).m_timestamp - _TimeCurrent;

    if (_StreamCurrent > stream_start)
    {
        filler.resize(_StreamCurrent - stream_start);
        filler.assign(&_Stream.at(stream_start), &_Stream.at(_StreamCurrent));

        unsigned long i, j;

        for (i = 0, stream_start = _StreamCurrent - stream_start; i < stream_start; i++)
        {
            midi_stream_event & e = filler.at(i);

            if ((e.m_event & 0x800000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
            {
                if ((e.m_event & 0x0F) == 9) // hax
                {
                    e.m_event = 0;
                    continue;
                }

                const uint32_t m = (e.m_event & 0x7F00FF0F) | 0x80; // note off
                const uint32_t m2 = (e.m_event & 0x7F00FFFF); // also note off

                for (j = i + 1; j < stream_start; j++)
                {
                    midi_stream_event & e2 = filler.at(j);
                    if ((e2.m_event & 0xFF00FFFF) == m || e2.m_event == m2)
                    {
                        // kill 'em
                        e.m_event = 0;
                        e2.m_event = 0;
                        break;
                    }
                }
            }
        }

        audio_sample * temp;
        const unsigned int needs_time = send_event_needs_time();

        if (needs_time)
        {
            temp = new audio_sample[needs_time * 2];

            if (temp)
            {
                render(temp, needs_time); // flush events

                unsigned int render_junk = 0;
                bool timestamp_set = false;
                unsigned last_timestamp = 0;

                for (i = 0; i < stream_start; i++)
                {
                    if (filler[i].m_event)
                    {
                        send_event_time_filtered(filler[i].m_event, render_junk);

                        if (timestamp_set)
                        {
                            if (filler[i].m_timestamp != last_timestamp)
                            {
                                render_junk += 16;
                            }
                        }

                        last_timestamp = filler[i].m_timestamp;
                        timestamp_set = true;

                        if (render_junk >= needs_time)
                        {
                            render(temp, needs_time);
                            render_junk -= needs_time;
                        }
                    }
                }

                render(temp, needs_time);

                delete[] temp;
            }
        }
        else
        {
            temp = new audio_sample[16 * 2];

            if (temp)
            {
                render(temp, 16);

                bool timestamp_set = false;
                unsigned last_timestamp = 0;

                for (i = 0; i < stream_start; i++)
                {
                    if (filler[i].m_event)
                    {
                        if (timestamp_set)
                        {
                            if (filler[i].m_timestamp != last_timestamp)
                            {
                                render(temp, 16);
                            }
                        }

                        last_timestamp = filler[i].m_timestamp;
                        timestamp_set = true;

                        send_event_filtered(filler[i].m_event);
                    }
                }

                render(temp, 16);

                delete[] temp;
            }
        }
    }
}

bool MIDIPlayer::GetLastError(std::string & p_out)
{
    return get_last_error(p_out);
}

void MIDIPlayer::setSampleRate(unsigned long sampleRate)
{
    if (_Stream.size())
        for (size_t i = 0; i < _Stream.size(); i++)
            _Stream.at(i).m_timestamp = (unsigned long) ((uint64_t) _Stream.at(i).m_timestamp * sampleRate / _SampleRate);

    if (_TimeCurrent)
        _TimeCurrent = static_cast<unsigned long>(static_cast<uint64_t>(_TimeCurrent) * sampleRate / _SampleRate);

    if (_TimeEnd)
        _TimeEnd = static_cast<unsigned long>(static_cast<uint64_t>(_TimeEnd) * sampleRate / _SampleRate);

    if (_LoopStartTime)
        _LoopStartTime = static_cast<unsigned long>(static_cast<uint64_t>(_LoopStartTime) * sampleRate / _SampleRate);

    _SampleRate = sampleRate;

    shutdown();
}

void MIDIPlayer::setLoopMode(unsigned int loopMode)
{
    if (_LoopMode == loopMode)
        return;

    if (loopMode & loop_mode_enable)
        _TimeEnd -= _SampleRate;
    else
        _TimeEnd += _SampleRate;

    _LoopMode = loopMode;
}

void MIDIPlayer::setFilterMode(filter_mode filterMode, bool useMIDIEffects)
{
    _FilterMode = filterMode;
    _UseMIDIEffects = useMIDIEffects;

    if (_IsInitialized)
    {
        sysex_reset(0, 0);
        sysex_reset(1, 0);
        sysex_reset(2, 0);
    }
}

void MIDIPlayer::send_event_filtered(uint32_t message)
{
    if (!(message & 0x80000000u))
    {
        if (_UseMIDIEffects)
        {
            const uint32_t _b = message & 0x7FF0;

            if (_b == 0x5BB0 || _b == 0x5DB0)
                return;
        }

        send_event(message);
    }
    else
    {
        const unsigned int p_index = message & 0xffffff;
        const uint8_t * p_data;
        size_t p_size, p_port;

        _SysExMap.get_entry(p_index, p_data, p_size, p_port);
        send_sysex_filtered(p_data, p_size, p_port);
    }
}

void MIDIPlayer::send_event_time_filtered(uint32_t message, unsigned int time)
{
    if (!(message & 0x80000000u))
    {
        if (_UseMIDIEffects)
        {
            const uint32_t _b = message & 0x7FF0;

            if (_b == 0x5BB0 || _b == 0x5DB0)
                return;
        }

        send_event_time(message, time);
    }
    else
    {
        const unsigned int p_index = message & 0xffffff;
        const uint8_t * p_data;
        size_t p_size, p_port;

        _SysExMap.get_entry(p_index, p_data, p_size, p_port);
        send_sysex_time_filtered(p_data, p_size, p_port, time);
    }
}

#pragma region("Sys Ex")
static const uint8_t syx_reset_gm[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t syx_reset_gm2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t syx_reset_gs[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t syx_reset_xg[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool syx_equal(const uint8_t * a, const uint8_t * b)
{
    while (*a != 0xF7 && *b != 0xF7 && *a == *b)
    {
        a++;
        b++;
    }

    return *a == *b;
}

static bool syx_is_reset(const uint8_t * data)
{
    return syx_equal(data, &syx_reset_gm[0]) || syx_equal(data, &syx_reset_gm2[0]) || syx_equal(data, &syx_reset_gs[0]) || syx_equal(data, &syx_reset_xg[0]);
}

void MIDIPlayer::sysex_send_gs(size_t port, uint8_t * data, size_t size, unsigned int time)
{
    size_t i;

    unsigned char checksum = 0;

    for (i = 5; i + 1 < size && data[i + 1] != 0xF7; ++i)
        checksum += data[i];

    data[i] = (unsigned char)((128 - checksum) & 127);

    if (time)
        send_sysex_time(data, size, port, time);
    else
        send_sysex(data, size, port);
}

void MIDIPlayer::sysex_reset_sc(size_t port, unsigned int time)
{
    uint8_t message[11];

    ::memcpy(&message[0], &syx_gs_limit_bank_lsb[0], sizeof(message));

    message[7] = 1;

    switch (_FilterMode)
    {
        case filter_sc55:
            message[8] = 1;
            break;

        case filter_sc88:
            message[8] = 2;
            break;

        case filter_sc88pro:
            message[8] = 3;
            break;

        case filter_sc8850:
        case filter_default:
            message[8] = 4;
            break;

        case filter_gm:
        case filter_gm2:
        case filter_xg:
        default:
            break;
    }

    for (uint8_t i = 0x41; i <= 0x49; ++i)
    {
        message[6] = i;
        sysex_send_gs(port, &message[0], sizeof(message), time);
    }

    {
        message[6] = 0x40;
        sysex_send_gs(port, &message[0], sizeof(message), time);
    }

    for (uint8_t i = 0x4A; i <= 0x4F; ++i)
    {
        message[6] = i;
        sysex_send_gs(port, &message[0], sizeof(message), time);
    }
}

void MIDIPlayer::sysex_reset(size_t port, unsigned int time)
{
    if (!_IsInitialized)
        return;

    if (time)
    {
        send_sysex_time(&syx_reset_xg[0], sizeof(syx_reset_xg), port, time);
        send_sysex_time(&syx_reset_gm2[0], sizeof(syx_reset_gm2), port, time);
        send_sysex_time(&syx_reset_gm[0], sizeof(syx_reset_gm), port, time);
    }
    else
    {
        send_sysex(&syx_reset_xg[0], sizeof(syx_reset_xg), port);
        send_sysex(&syx_reset_gm2[0], sizeof(syx_reset_gm2), port);
        send_sysex(&syx_reset_gm[0], sizeof(syx_reset_gm), port);
    }

    switch (_FilterMode)
    {
        case filter_gm:
            /*
            if (time)
                send_sysex_time(syx_reset_gm, sizeof(syx_reset_gm), port, time);
            else
                send_sysex(syx_reset_gm, sizeof(syx_reset_gm), port);
                */
            break;

        case filter_gm2:
            if (time)
                send_sysex_time(&syx_reset_gm2[0], sizeof(syx_reset_gm2), port, time);
            else
                send_sysex(&syx_reset_gm2[0], sizeof(syx_reset_gm2), port);
            break;

        case filter_sc55:
        case filter_sc88:
        case filter_sc88pro:
        case filter_sc8850:
        case filter_default:
            if (time)
                send_sysex_time(&syx_reset_gs[0], sizeof(syx_reset_gs), port, time);
            else
                send_sysex(&syx_reset_gs[0], sizeof(syx_reset_gs), port);
            sysex_reset_sc(port, time);
            break;

        case filter_xg:
            if (time)
                send_sysex_time(&syx_reset_xg[0], sizeof(syx_reset_xg), port, time);
            else
                send_sysex(&syx_reset_xg[0], sizeof(syx_reset_xg), port);
            break;
    }

    {
        for (size_t i = 0; i < 16; ++i)
        {
            if (time)
            {
                send_event_time((uint32_t)(0x78B0 + i + (port << 24)), time);
                send_event_time((uint32_t)(0x79B0 + i + (port << 24)), time);

                if (_FilterMode != filter_xg || i != 9)
                {
                    send_event_time((uint32_t)(0x20B0 + i + (port << 24)), time);
                    send_event_time((uint32_t)(0x00B0 + i + (port << 24)), time);
                    send_event_time((uint32_t)(0x00C0 + i + (port << 24)), time);
                }
            }
            else
            {
                send_event((uint32_t)(0x78B0 + i + (port << 24)));
                send_event((uint32_t)(0x79B0 + i + (port << 24)));

                if (_FilterMode != filter_xg || i != 9)
                {
                    send_event((uint32_t)(0x20B0 + i + (port << 24)));
                    send_event((uint32_t)(0x00B0 + i + (port << 24)));
                    send_event((uint32_t)(0x00C0 + i + (port << 24)));
                }
            }
        }
    }

    if (_FilterMode == filter_xg)
    {
        if (time)
        {
            send_event_time((uint32_t)(0x0020B9 + (port << 24)), time);
            send_event_time((uint32_t)(0x7F00B9 + (port << 24)), time);
            send_event_time((uint32_t)(0x0000C9 + (port << 24)), time);
        }
        else
        {
            send_event((uint32_t)(0x0020B9 + (port << 24)));
            send_event((uint32_t)(0x7F00B9 + (port << 24)));
            send_event((uint32_t)(0x0000C9 + (port << 24)));
        }
    }

    if (_UseMIDIEffects)
    {
        if (time)
        {
            for (size_t i = 0; i < 16; ++i)
            {
                send_event_time((uint32_t)(0x5BB0 + i + (port << 24)), time);
                send_event_time((uint32_t)(0x5DB0 + i + (port << 24)), time);
            }
        }
        else
        {
            for (size_t i = 0; i < 16; ++i)
            {
                send_event((uint32_t)(0x5BB0 + i + (port << 24)));
                send_event((uint32_t)(0x5DB0 + i + (port << 24)));
            }
        }
    }
}

void MIDIPlayer::send_sysex_filtered(const uint8_t * data, size_t size, size_t port)
{
    send_sysex(data, size, port);

    if (syx_is_reset(data) && _FilterMode != filter_default)
        sysex_reset(port, 0);
}

void MIDIPlayer::send_sysex_time_filtered(const uint8_t * data, size_t size, size_t port, unsigned int time)
{
    send_sysex_time(data, size, port, time);

    if (syx_is_reset(data) && _FilterMode != filter_default)
        sysex_reset(port, time);
}
#pragma endregion
