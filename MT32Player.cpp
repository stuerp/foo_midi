#include "MT32Player.h"

MT32Player::MT32Player(bool gm, unsigned gm_set) : bGM(gm), uGMSet(gm_set), MIDIPlayer()
{
    _synth = NULL;
    controlRom = NULL;
    pcmRom = NULL;
    controlRomFile = NULL;
    pcmRomFile = NULL;
}

MT32Player::~MT32Player()
{
    if (_synth)
    {
        _synth->close();
        delete _synth;
    }
    if (controlRom)
    {
        MT32Emu::ROMImage::freeROMImage(controlRom);
    }
    if (pcmRom)
    {
        MT32Emu::ROMImage::freeROMImage(pcmRom);
    }
    delete controlRomFile;
    delete pcmRomFile;
}

void MT32Player::send_event(uint32_t b)
{
    _synth->playMsg(b);
}

void MT32Player::send_sysex(const uint8_t * event, uint32_t size, size_t port)
{
    _synth->playSysexNow(event, size);
}

void MT32Player::render(audio_sample * out, unsigned long count)
{
#if 0
    pfc::static_assert_t<sizeof(audio_sample) == sizeof(float)>();
    _synth->render(out, count);
#else
    MT32Emu::Bit16s temp[256 * 2];

    while (count > 0)
    {
        unsigned long todo = count;

        if (todo > 256)
            todo = 256;

        _synth->render(temp, (unsigned) todo);

        audio_math::convert_from_int16(temp, (todo * 2), out, 1.);

        out += (todo * 2);
        count -= todo;
    }
#endif
}

void MT32Player::setBasePath(const char * in)
{
    sBasePath = in;
    shutdown();
}

void MT32Player::setAbortCallback(abort_callback * in)
{
    _abort = in;
}

void MT32Player::shutdown()
{
    if (_synth)
    {
        _synth->close();
        delete _synth;
    }
    if (controlRom)
    {
        MT32Emu::ROMImage::freeROMImage(controlRom);
    }
    if (pcmRom)
    {
        MT32Emu::ROMImage::freeROMImage(pcmRom);
    }
    delete controlRomFile;
    delete pcmRomFile;
    _synth = 0;
    controlRom = 0;
    pcmRom = 0;
    controlRomFile = 0;
    pcmRomFile = 0;
    _IsInitialized = false;
}

static const char * control_rom_names[] = { "CM32L_CONTROL.ROM", "MT32_CONTROL.ROM" };
static const char * pcm_rom_names[] = { "CM32L_PCM.ROM", "MT32_PCM.ROM" };

static const MT32Emu::AnalogOutputMode useMode = MT32Emu::AnalogOutputMode::AnalogOutputMode_ACCURATE;

bool MT32Player::startup()
{
    if (_synth) return true;

    unsigned rom_set = 0;
    controlRomFile = openFile(control_rom_names[0]);
    if (!controlRomFile)
    {
        rom_set = 1;
        controlRomFile = openFile(control_rom_names[1]);
    }
    if (!controlRomFile) return false;
    pcmRomFile = openFile(pcm_rom_names[rom_set]);
    if (!pcmRomFile) return false;
    controlRom = MT32Emu::ROMImage::makeROMImage(controlRomFile);
    if (!controlRom) return false;
    pcmRom = MT32Emu::ROMImage::makeROMImage(pcmRomFile);
    if (!pcmRom) return false;
    _synth = new MT32Emu::Synth;
    if (!_synth->open(*controlRom, *pcmRom, bGM ? 256U : 32U, useMode, bGM))
    {
        delete _synth;
        _synth = 0;
        return false;
    }
    _reset();
    _IsInitialized = true;
    return true;
}

int MT32Player::getSampleRate()
{
    return MT32Emu::Synth::getStereoOutputSampleRate(useMode);
}

bool MT32Player::isConfigValid()
{
    return startup();
}

void MT32Player::_reset()
{
    static const uint8_t mt32_reset[10] = { 0xF0, MT32Emu::SYSEX_MANUFACTURER_ROLAND, 0x10, MT32Emu::SYSEX_MDL_MT32, MT32Emu::SYSEX_CMD_DT1, 0x7F, 0, 0, 0xF7 };

    _synth->playSysexNow(mt32_reset, sizeof(mt32_reset));

    if (bGM)
    {
    #include "kq6mtgm.h"
    #include "mtgm.h"
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
            while (sequence_end < end && *sequence_end != 0xF7) sequence_end++;
            _synth->playSysexNow(start, sequence_end - start + 1);
            start = sequence_end + 1;
        }
    }
}

class FBArrayFile : public MT32Emu::ArrayFile
{
    MT32Emu::Bit8u * keptData;

public:
    FBArrayFile(MT32Emu::Bit8u * data, size_t size)
        : MT32Emu::ArrayFile(data, size), keptData(data)
    {
    }
    virtual ~FBArrayFile()
    {
        delete[] keptData;
    }
};

MT32Emu::File * MT32Player::openFile(const char * filename)
{
    MT32Emu::Bit8u * data = NULL;
    try
    {
        service_ptr_t<file> p_temp;
        pfc::string8 tempname;
        tempname = sBasePath;
        tempname += filename;
        filesystem::g_open(p_temp, tempname, filesystem::open_mode_read, *_abort);
        t_filesize length64 = p_temp->get_size_ex(*_abort);
        if (length64 > INT_MAX) length64 = INT_MAX;
        size_t fileSize = length64;
        data = new MT32Emu::Bit8u[length64];
        p_temp->read_object(data, fileSize, *_abort);
        return new FBArrayFile(data, fileSize);
    }
    catch (...)
    {
        delete[] data;
        return NULL;
    }
}
