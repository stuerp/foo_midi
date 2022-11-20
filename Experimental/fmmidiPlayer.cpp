#include <foobar2000.h>

#include "fmmidiPlayer.h"

#include <fmmidi.hpp>
#include <midisynth.hpp>

#include <stdio.h>

fmmidiPlayer::fmmidiPlayer() : MIDIPlayer()
{
    factory = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        synthesizers[i] = 0;
    }
}

fmmidiPlayer::~fmmidiPlayer()
{
    shutdown();
}

void fmmidiPlayer::send_event(uint32_t b)
{
    if (!(b & 0x80000000))
    {
        unsigned port = (b >> 24) & 0x7F;
        synthesizers[port & 3]->midi_event(b);
    }
    else
    {
        UINT n = b & 0xffffff;
        const t_uint8 * data;
        t_size size, port;
        mSysexMap.get_entry(n, data, size, port);
        synthesizers[port & 3]->sysex_message(data, size);
    }
}

void fmmidiPlayer::render(float * out, unsigned long count)
{
    int_least32_t buffer[512];

    while (count)
    {
        unsigned todo = count;
        if (todo > 256) todo = 256;

        memset(buffer, 0, todo * sizeof(*buffer) * 2);

        for (unsigned i = 0; i < 4; i++)
        {
            synthesizers[i]->synthesize_mixing(buffer, todo, uSampleRate);
        }

        audio_math::convert_from_int32((const t_int32 *) buffer, todo * 2, out, 65536.0);

        out += todo * 2;
        count -= todo;
    }
}

void fmmidiPlayer::setProgramPath(const char * path)
{
    bank_path = path;
    bank_path += "programs.txt";
}

void fmmidiPlayer::shutdown()
{
    for (unsigned i = 0; i < 4; i++)
    {
        delete synthesizers[i];
        synthesizers[i] = NULL;
    }
    delete factory;
    factory = NULL;
}

bool fmmidiPlayer::startup()
{
    if (factory) return true;

    factory = new midisynth::opn::fm_note_factory;

    FILE * fp = _tfopen(pfc::stringcvt::string_os_from_utf8(bank_path.c_str()), _T("rt"));
    if (!fp)
    {
        return false;
    }
    else
    {
        while (!feof(fp))
        {
            int c = getc(fp);
            if (c == '@')
            {
                int prog;
                midisynth::opn::FMPARAMETER p;
                if (fscanf(fp, "%d%d%d%d", &prog, &p.ALG, &p.FB, &p.LFO) == 4 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op1.AR, &p.op1.DR, &p.op1.SR, &p.op1.RR, &p.op1.SL, &p.op1.TL, &p.op1.KS, &p.op1.ML, &p.op1.DT, &p.op1.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op2.AR, &p.op2.DR, &p.op2.SR, &p.op2.RR, &p.op2.SL, &p.op2.TL, &p.op2.KS, &p.op2.ML, &p.op2.DT, &p.op2.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op3.AR, &p.op3.DR, &p.op3.SR, &p.op3.RR, &p.op3.SL, &p.op3.TL, &p.op3.KS, &p.op3.ML, &p.op3.DT, &p.op3.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op4.AR, &p.op4.DR, &p.op4.SR, &p.op4.RR, &p.op4.SL, &p.op4.TL, &p.op4.KS, &p.op4.ML, &p.op4.DT, &p.op4.AMS) == 10)
                {
                    factory->set_program(prog, p);
                }
            }
            else if (c == '*')
            {
                int prog;
                midisynth::opn::DRUMPARAMETER p;
                if (fscanf(fp, "%d%d%d%d%d%d%d", &prog, &p.ALG, &p.FB, &p.LFO, &p.key, &p.panpot, &p.assign) == 7 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op1.AR, &p.op1.DR, &p.op1.SR, &p.op1.RR, &p.op1.SL, &p.op1.TL, &p.op1.KS, &p.op1.ML, &p.op1.DT, &p.op1.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op2.AR, &p.op2.DR, &p.op2.SR, &p.op2.RR, &p.op2.SL, &p.op2.TL, &p.op2.KS, &p.op2.ML, &p.op2.DT, &p.op2.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op3.AR, &p.op3.DR, &p.op3.SR, &p.op3.RR, &p.op3.SL, &p.op3.TL, &p.op3.KS, &p.op3.ML, &p.op3.DT, &p.op3.AMS) == 10 && fscanf(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op4.AR, &p.op4.DR, &p.op4.SR, &p.op4.RR, &p.op4.SL, &p.op4.TL, &p.op4.KS, &p.op4.ML, &p.op4.DT, &p.op4.AMS) == 10)
                {
                    factory->set_drum_program(prog, p);
                }
            }
        }
        fclose(fp);
    }

    for (unsigned i = 0; i < 4; i++)
    {
        synthesizers[i] = new midisynth::synthesizer(factory);
    }

    return true;
}
