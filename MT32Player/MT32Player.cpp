
/** MT32Player.cpp (2013.01.15) **/

#include "MT32Player.h"

#include "kq6mtgm.h"
#include "mtgm.h"

#pragma warning(disable: 5045)

#include <pfc/pathUtils.h>

MT32Player::MT32Player(bool isGM, unsigned gmSet) : MIDIPlayer(), _GMSet(gmSet), _IsGM(isGM)
{
    _Synth = nullptr;

    controlRom = nullptr;
    pcmRom = nullptr;
    controlRomFile = nullptr;
    pcmRomFile = nullptr;
}

MT32Player::~MT32Player()
{
    Shutdown();
}

static const MT32Emu::AnalogOutputMode useMode = MT32Emu::AnalogOutputMode::AnalogOutputMode_ACCURATE;

static const char * ControlROMNames[] =
{
    "CM32L_CONTROL.ROM",
    "MT32_CONTROL.ROM"
};

static const char * PCMROMNames[] =
{
    "CM32L_PCM.ROM",
    "MT32_PCM.ROM"
};

#pragma region("MIDIPlayer")
bool MT32Player::Startup()
{
    if (_Synth)
        return true;

    unsigned rom_set = 0;

    controlRomFile = openFile(ControlROMNames[0]);

    if (!controlRomFile)
    {
        rom_set = 1;
        controlRomFile = openFile(ControlROMNames[1]);
    }

    if (!controlRomFile)
        return false;

    pcmRomFile = openFile(PCMROMNames[rom_set]);

    if (!pcmRomFile)
        return false;

    controlRom = MT32Emu::ROMImage::makeROMImage(controlRomFile);

    if (!controlRom)
        return false;

    pcmRom = MT32Emu::ROMImage::makeROMImage(pcmRomFile);

    if (!pcmRom)
        return false;

    _Synth = new MT32Emu::Synth;

    MT32Emu::Bit32u UsePartialCount = _IsGM ? 256U : 32U;

    if (!_Synth->open(*controlRom, *pcmRom, UsePartialCount, useMode))
    {
        delete _Synth;
        _Synth = 0;
        return false;
    }

    _reset();

    _IsInitialized = true;

    return true;
}

void MT32Player::Shutdown()
{
    if (_Synth)
    {
        _Synth->close();

        delete _Synth;
        _Synth = nullptr;
    }

    if (controlRom)
    {
        MT32Emu::ROMImage::freeROMImage(controlRom);
        controlRom = nullptr;
    }

    if (pcmRom)
    {
        MT32Emu::ROMImage::freeROMImage(pcmRom);
        pcmRom = nullptr;
    }

    delete controlRomFile;
    controlRomFile = nullptr;

    delete pcmRomFile;
    pcmRomFile = nullptr;

    _IsInitialized = false;
}

void MT32Player::Render(audio_sample * samples, unsigned long count)
{
    MT32Emu::Bit16s temp[256 * 2];

    while (count > 0)
    {
        unsigned long ToDo = count;

        if (ToDo > 256)
            ToDo = 256;

        _Synth->render(temp, (unsigned) ToDo);

        audio_math::convert_from_int16(temp, (ToDo * 2), samples, 1.);

        samples += (ToDo * 2);
        count -= ToDo;
    }
}

void MT32Player::SendEvent(uint32_t b)
{
    _Synth->playMsg(b);
}

void MT32Player::SendSysEx(const uint8_t * event, size_t size, size_t)
{
    _Synth->playSysexNow(event, (MT32Emu::Bit32u)size);
}
#pragma endregion

void MT32Player::setBasePath(const char * in)
{
    _BasePathName = in;

    Shutdown();
}

int MT32Player::GetSampleRate()
{
    return (int)MT32Emu::Synth::getStereoOutputSampleRate(useMode);
}

bool MT32Player::isConfigValid()
{
    return Startup();
}

void MT32Player::_reset()
{
    static const uint8_t mt32_reset[10] = { 0xF0, MT32Emu::SYSEX_MANUFACTURER_ROLAND, 0x10, MT32Emu::SYSEX_MDL_MT32, MT32Emu::SYSEX_CMD_DT1, 0x7F, 0, 0, 0xF7 };

    _Synth->playSysexNow(mt32_reset, sizeof(mt32_reset));

    if (_IsGM)
    {
        const unsigned char * start, * end;

        if (_GMSet == 0)
        {
            start = mt32_gm_sysex;
            end = start + _countof(mt32_gm_sysex);
        }
        else
        {
            start = kq6_mt32_gm_sysex;
            end = start + _countof(kq6_mt32_gm_sysex);
        }

        while (start < end)
        {
            const unsigned char * sequence_end = start;

            while (sequence_end < end && *sequence_end != 0xF7)
                sequence_end++;

            _Synth->playSysexNow(start, (MT32Emu::Bit32u)(sequence_end - start + 1));

            start = sequence_end + 1;
        }
    }
}

class FBArrayFile : public MT32Emu::ArrayFile
{
public:
    FBArrayFile(MT32Emu::Bit8u * data, size_t size) : MT32Emu::ArrayFile(data, size), _Data(data)
    {
    }

    virtual ~FBArrayFile()
    {
        delete[] _Data;
    }

private:
    MT32Emu::Bit8u * _Data;
};

MT32Emu::File * MT32Player::openFile(const char * fileName)
{
    MT32Emu::Bit8u * Data = nullptr;

    try
    {
        service_ptr_t<file> File;

        pfc::string8 FilePath = pfc::io::path::combine(_BasePathName, fileName);

        filesystem::g_open(File, FilePath, filesystem::open_mode_read, *_AbortCallback);

        t_filesize FileSize = File->get_size_ex(*_AbortCallback);

        if (FileSize > INT_MAX)
            FileSize = INT_MAX;

        Data = new MT32Emu::Bit8u[FileSize];

        File->read_object(Data, FileSize, *_AbortCallback);

        return new FBArrayFile(Data, FileSize);
    }
    catch (...)
    {
        delete[] Data;

        return nullptr;
    }
}
