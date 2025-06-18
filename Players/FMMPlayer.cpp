
/** $VER: FMMPlayer.cpp (2025.06.18) - Wrapper for yuno's fmmidi **/

#include "framework.h"

#include "FMMPlayer.h"

#include <fmmidi.hpp>
#include <midisynth.hpp>

#pragma region player_t

FMMPlayer::FMMPlayer() : player_t()
{
    _Factory = nullptr;

    for (size_t i = 0; i < _countof(_Synthesizers); ++i)
        _Synthesizers[i] = nullptr;
}

FMMPlayer::~FMMPlayer()
{
    Shutdown();
}

bool FMMPlayer::Startup()
{
    if (_Factory)
        return true;

    _Factory = new midisynth::opn::fm_note_factory;

    FILE * fp = nullptr;

    if (::_wfopen_s(&fp, pfc::stringcvt::string_os_from_utf8(_ProgramPath.c_str()), L"rt") != 0)
         throw std::runtime_error("Unable to find \"Programs.txt\"");

    while (!::feof(fp))
    {
        int c = ::getc(fp);

        if (c == '@')
        {
            int Program;
            midisynth::opn::FMPARAMETER p;

            if (::fscanf_s(fp, "%d%d%d%d", &Program, &p.ALG, &p.FB, &p.LFO) == 4 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op1.AR, &p.op1.DR, &p.op1.SR, &p.op1.RR, &p.op1.SL, &p.op1.TL, &p.op1.KS, &p.op1.ML, &p.op1.DT, &p.op1.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op2.AR, &p.op2.DR, &p.op2.SR, &p.op2.RR, &p.op2.SL, &p.op2.TL, &p.op2.KS, &p.op2.ML, &p.op2.DT, &p.op2.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op3.AR, &p.op3.DR, &p.op3.SR, &p.op3.RR, &p.op3.SL, &p.op3.TL, &p.op3.KS, &p.op3.ML, &p.op3.DT, &p.op3.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op4.AR, &p.op4.DR, &p.op4.SR, &p.op4.RR, &p.op4.SL, &p.op4.TL, &p.op4.KS, &p.op4.ML, &p.op4.DT, &p.op4.AMS) == 10)
            {
                _Factory->set_program(Program, p);
            }
        }
        else
        if (c == '*')
        {
            int Program;
            midisynth::opn::DRUMPARAMETER p;

            if (::fscanf_s(fp, "%d%d%d%d%d%d%d", &Program, &p.ALG, &p.FB, &p.LFO, &p.key, &p.panpot, &p.assign) == 7 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op1.AR, &p.op1.DR, &p.op1.SR, &p.op1.RR, &p.op1.SL, &p.op1.TL, &p.op1.KS, &p.op1.ML, &p.op1.DT, &p.op1.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op2.AR, &p.op2.DR, &p.op2.SR, &p.op2.RR, &p.op2.SL, &p.op2.TL, &p.op2.KS, &p.op2.ML, &p.op2.DT, &p.op2.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op3.AR, &p.op3.DR, &p.op3.SR, &p.op3.RR, &p.op3.SL, &p.op3.TL, &p.op3.KS, &p.op3.ML, &p.op3.DT, &p.op3.AMS) == 10 &&
                ::fscanf_s(fp, "%d%d%d%d%d%d%d%d%d%d", &p.op4.AR, &p.op4.DR, &p.op4.SR, &p.op4.RR, &p.op4.SL, &p.op4.TL, &p.op4.KS, &p.op4.ML, &p.op4.DT, &p.op4.AMS) == 10)
            {
                _Factory->set_drum_program(Program, p);
            }
        }
    }

    ::fclose(fp);

    for (size_t i = 0; i < _countof(_Synthesizers); ++i)
        _Synthesizers[i] = new midisynth::synthesizer(_Factory);

    return true;
}

void FMMPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Synthesizers); ++i)
    {
        delete _Synthesizers[i];
        _Synthesizers[i] = nullptr;
    }

    delete _Factory;
    _Factory = nullptr;
}

void FMMPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    int32_t Data[512];

    while (sampleCount != 0)
    {
        uint32_t ToDo = sampleCount;

        if (ToDo > 256)
            ToDo = 256;

        ::memset(Data, 0, ToDo * 2 * sizeof(*Data));

        for (size_t i = 0; i < _countof(_Synthesizers); ++i)
            _Synthesizers[i]->synthesize_mixing(Data, ToDo, (float) _SampleRate);

        audio_math::convert_from_int32((const t_int32 *) Data, ToDo * 2, sampleData, 65536.0);

        sampleData  += ToDo * 2;
        sampleCount -= ToDo;
    }
}

void FMMPlayer::SendEvent(uint32_t data)
{
    if (!(data & 0x80000000u))
    {
        size_t Port = (data >> 24) & 0x7F;

        _Synthesizers[Port & 3]->midi_event(data);
    }
    else
    {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t * Data;
        size_t Size;
        uint8_t Port;

        _SysExMap.GetItem(Index, Data, Size, Port);

        _Synthesizers[Port & 3]->sysex_message(Data, Size);
    }
}

/// <summary>
/// Sets the path to the program file.
/// </summary>
void FMMPlayer::SetProgramPath(const std::string & programPath)
{
    _ProgramPath = pfc::io::path::combine(programPath.c_str(), "Programs.txt");
}
