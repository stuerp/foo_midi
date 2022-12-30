#pragma once

static const char field_hash[] = "midi_hash";
static const char field_format[] = "midi_format";
static const char field_tracks[] = "midi_tracks";
static const char field_channels[] = "midi_channels";
static const char field_ticks[] = "midi_ticks";
static const char field_type[] = "midi_type";
static const char field_loop_start[] = "midi_loop_start";
static const char field_loop_end[] = "midi_loop_end";
static const char field_loop_start_ms[] = "midi_loop_start_ms";
static const char field_loop_end_ms[] = "midi_loop_end_ms";
static const char field_preset[] = "midi_preset";
static const char field_syx[] = "midi_sysex_dumps";

static const GUID guid_midi_index = { 0x4209c12e, 0xc2f4, 0x40ca, { 0xb2, 0xbc, 0xfb, 0x61, 0xc3, 0x26, 0x87, 0xd0 } };

static const char * _FileExtension[] =
{
    "MID",
    "MIDI",
    "KAR",
    "RMI",
    "MIDS",
    "MDS",
    //	"CMF",
    //	"GMF",
    "HMI",
    "HMP",
    "HMQ",
    "MUS",
    "XMI",
    "LDS",
};

static const char * _SyxExtension[] =
{
    "SYX",
    "DMP"
};

static bool g_test_extension_syx(const char * ext)
{
    for (size_t n = 0; n < _countof(_SyxExtension); ++n)
    {
        if (!_stricmp(ext, _SyxExtension[n]))
            return true;
    }

    return false;
}

static const char * MuntBankNames[] =
{
    "Roland",
    "Sierra / King's Quest 6",
};
