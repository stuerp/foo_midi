#include "MT32Player.h"

#include "kq6mtgm.h"
#include "mtgm.h"

#pragma warning(disable: 5045)

MT32Player::MT32Player(bool gm, unsigned gm_set) : MIDIPlayer(), uGMSet(gm_set), bGM(gm)
{
    _Synth = nullptr;

    controlRom = nullptr;
    pcmRom = nullptr;
    controlRomFile = nullptr;
    pcmRomFile = nullptr;
}

MT32Player::~MT32Player()
{
    shutdown();
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

bool MT32Player::startup()
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

    MT32Emu::Bit32u UsePartialCount = bGM ? 256U : 32U;

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

void MT32Player::shutdown()
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

void MT32Player::send_event(uint32_t b)
{
    _Synth->playMsg(b);
}

void MT32Player::send_sysex(const uint8_t * event, size_t size, size_t)
{
    _Synth->playSysexNow(event, (MT32Emu::Bit32u)size);
}

void MT32Player::render(audio_sample * out, unsigned long count)
{
    MT32Emu::Bit16s temp[256 * 2];

    while (count > 0)
    {
        unsigned long todo = count;

        if (todo > 256)
            todo = 256;

        _Synth->render(temp, (unsigned) todo);

        audio_math::convert_from_int16(temp, (todo * 2), out, 1.);

        out += (todo * 2);
        count -= todo;
    }
}

void MT32Player::setBasePath(const char * in)
{
    _BasePathName = in;

    shutdown();
}

void MT32Player::setAbortCallback(abort_callback * in)
{
    _AbortCallback = in;
}

int MT32Player::GetSampleRate()
{
    return (int)MT32Emu::Synth::getStereoOutputSampleRate(useMode);
}

bool MT32Player::isConfigValid()
{
    return startup();
}

void MT32Player::_reset()
{
    static const uint8_t mt32_reset[10] = { 0xF0, MT32Emu::SYSEX_MANUFACTURER_ROLAND, 0x10, MT32Emu::SYSEX_MDL_MT32, MT32Emu::SYSEX_CMD_DT1, 0x7F, 0, 0, 0xF7 };

    _Synth->playSysexNow(mt32_reset, sizeof(mt32_reset));

    if (bGM)
    {
        const unsigned char * start, * end;

        if (uGMSet == 0)
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

        pfc::string8 FilePath;

        FilePath = _BasePathName;
        FilePath += fileName;

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

        return NULL;
    }
}
