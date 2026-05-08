
/** $VER: SysEx.cpp (2026.04.10) P. Stuer **/

#include "pch.h"

#include "SysEx.h"
#include "Tables.h"

#include <Encoding.h>

namespace midi
{

const uint8_t sysex_t::GM1SystemOn[6] =
{
    0xF0, // Start of SysEx
    0x7E, 0x7F, 0x09, 0x01,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::GM1SystemOff[6] =
{
    0xF0, // Start of SysEx
    0x7E, 0x7F, 0x09, 0x02,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::GM2SystemOn[6] =
{
    0xF0, // Start of SysEx
    0x7E, 0x7F, 0x09, 0x03,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::D50Reset[10] =
{
    0xF0, // Start of SysEx
    0x41, // Manufacturer ID (Roland)
    0x10, // Device ID
    0x14, // Model ID (D50)
    0x12, // Command ID (Data Set 1, DT1)
    0x7F, // Address MSB
    0x00,
    0x00,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::MT32Reset[10] =
{
    0xF0, // Start of SysEx
    0x41, // Manufacturer ID (Roland)
    0x10, // Device ID
    0x16, // Model ID (MT32)
    0x12, // Command ID (Data Set 1, DT1)
    0x7F, // Address MSB
    0x01, // Address LSB
    0xF7  // End of SysEx
};

const uint8_t sysex_t::GSReset[11] =
{
    0xF0, // Start of SysEx
    0x41, // Manufacturer ID (Roland)
    0x10, // Device ID
    0x42, // Model ID (GS)
    0x12, // Command ID (Data Set 1, DT1)
    0x40, // Address MSB
    0x00, // Address
    0x7F, // Address LSB
    0x00, // Data (GS Reset)
    0x41, // Checksum
    0xF7  // End of SysEx
};

const uint8_t sysex_t::GSToneMapNumber[11]  =
{
    0xF0, // Start of SysEx
    0x41, // Manufacturer ID (Roland)
    0x10, // Device ID
    0x42, // Model ID (GS)
    0x12, // Command ID (Data Set 1, DT1)
    0x40, // Tone Map Number?
    0x41, // Tone Map?
    0x00,
    0x03, // Channel?
    0x00, // Checksum
    0xF7  // End of SysEx
};

const uint8_t sysex_t::XGSystemOn[9] =
{
    0xF0, // Start of SysEx
    0x43, // Manufacturer ID (Yamaha)
    0x10, // Device ID
    0x4C, // Model ID
    0x00,
    0x00,
    0x7E,
    0x00,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::XGReset[9] =
{
    0xF0, // Start of SysEx
    0x43, // Manufacturer ID (Yamaha)
    0x10, // Device ID
    0x4C, // Model ID
    0x00,
    0x00,
    0x7F,
    0x00,
    0xF7  // End of SysEx
};

const uint8_t sysex_t::DLSOn[6] =
{
    0xF0, // Start of SysEx
    0x7E, // Universal Non-Real Time
    0x7F, // Device ID (7F = Broadcast)
    0x0A, // DLS message
    0x01, // DLS On
    0xF7  // End of SysEx
};

const uint8_t sysex_t::DLSOff[6] =
{
    0xF0, // Start of SysEx
    0x7E, // Universal Non-Real Time
    0x7F, // Device ID (7F = Broadcast)
    0x0A, // DLS message
    0x02, // DLS Off
    0xF7  // End of SysEx
};

const uint8_t sysex_t::DLSStaticVoiceAllocationOff[6] =
{
    0xF0, // Start of SysEx
    0x7E, // Universal Non-Real Time
    0x7F, // Device ID (7F = Broadcast)
    0x0A, // DLS message
    0x03, // DLS Static Voice Allocation Off
    0xF7  // End of SysEx
};

const uint8_t sysex_t::DLSStaticVoiceAllocationOn[6] =
{
    0xF0, // Start of SysEx
    0x7E, // Universal Non-Real Time
    0x7F, // Device ID (7F = Broadcast)
    0x0A, // DLS message
    0x04, // DLS Static Voice Allocation On
    0xF7  // End of SysEx
};

void sysex_t::Identify() noexcept
{
    assert(_Data[0] == 0xF0);

    _Iter = _Data.begin() + 1;

    IsChecksumValid = true;

    const uint32_t ManufacturerId = IdentifyManufacturer();

    _Iter++; // Skip device id.

    switch (ManufacturerId)
    {
        case 0x41: IdentifyRoland(); break;
        case 0x43: IdentifyYamaha(); break;

        // Universal Non-Real Time
        case 0x7E:
        {
            switch (_Data[3])
            {
                case 0x06:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Identity Request"; break;
                        case 0x02: Description = "Identity Reply"; break;
                        default:
                            Description = "<Invalid>";
                    }
                    break;
                }

                // Universal Non-Real Time (General MIDI, 1991)
                case 0x09:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "GM1 System On"; break;
                        case 0x02: Description = "GM1 System Off"; break;

                        // Universal Non-Real Time (General MIDI Level 2, 1999)
                        case 0x03: Description = "GM2 System On"; break;
                        default:
                            Description = "<Invalid>";
                    }
                    break;
                }

                // DLS Level 2.2, 1.16 DLS System Exclusive Messages
                case 0x0A:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "DLS On"; break;
                        case 0x02: Description = "DLS Off"; break;
                        case 0x03: Description = "DLS Static Voice Allocation Off"; break;
                        case 0x04: Description = "DLS Static Voice Allocation On"; break;
                        default:
                            Description = "<Invalid>";
                    }
                    break;
                }
            }
            break;
        }

        //  Universal Real Time
        case 0x7F:
        {
            switch (_Data[3])
            {
                // Universal Real Time
                case 0x03:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Bar Number"; break;
                        case 0x02: Description = "Time Signature (Immediate)"; break;
                        case 0x03: Description = "Time Signature (Delayed)"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                // Universal Real Time (General MIDI Level 2, 1999)
                case 0x04:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Master Volume"; break;
                        case 0x02: Description = "Master Balance"; break;
                        case 0x03: Description = "Master Fine Tune"; break;
                        case 0x04: Description = "Master Coarse Tune"; break;
                        case 0x05:
                        {
                            if (::memcmp (&_Data[5], "\x01\x01\x01\x01\x01", 5) == 0)
                                Description = "Global Reverb Parameters";
                            else
                            if (::memcmp (&_Data[5], "\x01\x01\x01\x01\x02", 5) == 0)
                                Description = "Global Chorus Parameters";
                            break;
                        }
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                case 0x05:
                {
                    Description = "Real-Time MTC Cueing Command";
/*
    { "\xF0\x7F" "\x00" "\x05",                                          4, L"Real-Time MTC Cueing Command" },
    { "\xF0\x7F" "\x00" "\x06",                                          4, L"MIDI Machine Control Command" },
    { "\xF0\x7F" "\x00" "\x07",                                          4, L"MIDI Machine Control Response" },
*/
                    break;
                }

                case 0x06:
                {
                    Description = "MIDI Machine Control Command";
                    break;
                }

                case 0x07:
                {
                    Description = "MIDI Machine Control Response";
                    break;
                }

                case 0x08:
                {
                    switch (_Data[4])
                    {
                        case 0x02: Description = "Single Note Tuning Change"; break;

                        case 0x07: Description = "Single Note Tuning Change with Bank Select"; break;
                        case 0x08: Description = "Scale/Octave Tuning Adjust"; break;
                        case 0x09: Description = "Scale/Octave Tuning Adjust (2-byte format)"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                case 0x09:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Channel Pressure (Aftertouch)"; break;
                        case 0x03: Description = "Controller (Control Change)"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                case 0x0A:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Key-based Instrument Controller"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                case 0x0B:
                {
                    switch (_Data[4])
                    {
                        case 0x01: Description = "Scalable Polyphony MIDI MIP Message"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                case 0x0C:
                {
                    switch (_Data[4])
                    {
                        case 0x00: Description = "Mobile Phone Control Message"; break;
                        default:
                            ::DebugBreak();
                    }
                    break;
                }

                default:
                    ::DebugBreak();
            }
            break;
        }

        default:
        {
            Description  = "Unknown";
//          ::DebugBreak();
            break;
        }
    }
}

/// <summary>
/// Identifies the manufacturer.
/// </summary>
uint32_t sysex_t::IdentifyManufacturer() noexcept
{
    uint32_t Id = *_Iter++; // 1-byte Id

    if (Id == 0x00)
        Id = ((uint32_t) *_Iter++ << 8) | *_Iter++; // 3-byte Id

    auto it = Manufacturers.find(Id);

    if (it != Manufacturers.end())
        Manufacturer = it->second;
    else
        Manufacturer = "Unknown";

    return Id;
}

/// <summary>
/// Identifies a Roland SysEx.
/// </summary>
void sysex_t::IdentifyRoland() noexcept
{
    Manufacturer = "Roland";

    // Model ID
    Model = "Unknown";

    if (*_Iter == 0xF7)
    {
        Description = "Invalid SysEx";

        return;
    }

    switch (*_Iter++)
    {
        case 0x0B: Model = "MC505/JX305/D2"; break;
        case 0x10: Model = "S-10"; break;
        case 0x14: Model = "MT-32"; break;      // Multi-timbral sound module
        case 0x15: Model = "D-110"; break;      // Multi-timbral sound module
        case 0x16: Model = "D-50"; break;       // D-50, D-550, D-110
        case 0x1A: Model = "C-50/20"; break;
        case 0x2A: Model = "TR-808"; break;     // Rhythm machine
        case 0x2B: Model = "TR-909"; break;     // Rhythm machine
        case 0x2C: Model = "JV-1080"; break;
        case 0x2D: Model = "JV-1080"; break;
        case 0x2E: Model = "XP-30"; break;
        case 0x2F: Model = "XP-60"; break;
        case 0x30: Model = "XP-80"; break;
        case 0x38: Model = "RSP-550"; break;    // Multi-effects unit
        case 0x3C: Model = "XV-5080"; break;
        case 0x3D: Model = "JD-800"; break;
        case 0x3E: Model = "Fantom-S"; break;
        case 0x3F: Model = "Fantom-X"; break;
        case 0x40: Model = "V-Synth"; break;
        case 0x41: Model = "V-Synth-XT"; break;

        case 0x42: Model = "GS"; break;         // GS-compatible device (e.g. SC-88, V-Synth-GT)

        case 0x44: Model = "SH-201"; break;
        case 0x45: Model = "SC-55"; break;      // SC-55, SC-155, SC-88, SC-88 Pro, Juno-G
        case 0x46: Model = "Juno-Stage"; break;
        case 0x47: Model = "Juno-Di"; break;
        case 0x48: Model = "Juno-Gi"; break;
        case 0x49: Model = "Gaia SH-01"; break;
        case 0x4A: Model = "Jupiter-80"; break;
        case 0x4B: Model = "Jupiter-50"; break;
        case 0x4C: Model = "Integra-7"; break;
        case 0x4D: Model = "FA-06/FA-08"; break;
        case 0x4E: Model = "System-1"; break;
        case 0x4F: Model = "System-8"; break;
        case 0x50: Model = "TR-8"; break;
        case 0x51: Model = "TR-8S"; break;
        case 0x52: Model = "MC-707"; break;
        case 0x53: Model = "MC-101"; break;
        case 0x54: Model = "Zen-Core"; break;

        case 0x56: Model = "SC-7"; break;
        case 0x57: Model = "JD-990"; break;
        case 0x5C: Model = "Unknown"; break;

        case 0x6A: Model = "JV-1010"; break;
        case 0x6B: Model = "Fantom XR"; break;

        case 0x72: Model = "FC-200"; break;     // MIDI Foot Controller

        case 0x7C: Model = "VS-880"; break;

        case 0x00:
        {
            // 00 xx
            switch (*_Iter++)
            {
                case 0x07: Model = "GR-30"; break;
                case 0x0B: Model = "D-2"; break;
                case 0x3F: Model = "TD-6"; break;
                case 0x48: Model = "SD-20"; break;          // SD-20, SD-90
                case 0x51: Model = "V-Link"; break;         // HandSonic HPD-20
                case 0x6B: Model = "Fantom-Xx"; break;      // Fantom-X6, Fantom-X7, Fantom-X8

                // 00 00 xx
                case 0x00:
                {
                    switch (*_Iter++)
                    {
                        case 0x02: Model = "DR-880"; break; // 2005, Dr. Rythm
                        case 0x0B: Model = "MC-707"; break; // Groovebox, used for track and pattern data.
                        case 0x0C: Model = "MC-101"; break; // Compact groovebox, similar to MC-707.

                        case 0x16: Model = "SH-201"; break;
                        case 0x1C: Model = "VG-99"; break;
                        case 0x1E: Model = "SH-4d"; break;  // Desktop synthesizer, used for patch and sequence data.
                        case 0x20: Model = "V-1HD"; break;  // Video switcher, uses a 4-byte Model ID.

                        case 0x29: Model = "TR-06"; break;  // Used for Roland Boutique synths. Also JU-08A.
                        case 0x2D: Model = "TR-8S"; break;  // Drum machine, used for pattern and kit data.

                        case 0x3A: Model = "Juno-DS"; break;
                        case 0x3F: Model = "Integra-7"; break;

                        case 0x45: Model = "Fantom (2019)"; break;
                        case 0x4A: Model = "Jupiter-X/Xm"; break;
                        case 0x64: Model = "Integra-7"; break;

                        // 00 00 00 xx
                        case 0x00:
                        {
                            switch (*_Iter++)
                            {
                                case 0x44: Model = "SE-02"; break;
                                case 0x45: Model = "TR-8S"; break;
                                case 0x5B: Model = "Fantom-06/07/08"; break; // 2022
                                case 0x63: Model = "TD-27"; break; // 2020
                                case 0x65: Model = "Juniper-X"; break;
                                case 0xC0: Model = "MC-707"; break;

                                // 00 00 00 00 xx
                                case 0x00:
                                {
                                    switch (*_Iter++)
                                    {
                                        case 0xCF: Model = "Verselab MV-1"; break;

                                        default: Model = "Unknown"; break;
                                    }
                                }

                                default: Model = "Unknown"; break;
                            }
                            break;
                        }

                        default: Model = "Unknown"; break;
                    }
                    break;
                }

                default: Model = "Unknown"; break;
            }
            break;
        }
    
        default: Model = "Unknown"; break;
    }

    if (*_Iter == 0xF7)
    {
        Description = "Invalid SysEx";

        return;
    }

    // Command ID
    switch (*_Iter++)
    {
        case 0x10: Command = "REQ"; break;
        case 0x11: Command = "RQ1"; break; // Data Request 1
        case 0x12: Command = "DT1"; break; // Data Set 1
        case 0x13: Command = "ACK"; break;
        case 0x14: Command = "NAK"; break;
        default:   Command = "???"; break;
    }

    if (*_Iter == 0xF7)
    {
        Description = "Invalid SysEx";

        return;
    }

    uint32_t Address = (uint32_t) (_Iter[0] << 16) | (_Iter[1] << 8) | _Iter[2];

    Description = "Unknown";

    switch (_Iter[0])
    {
        // System parameters. Parameters affecting the entire unit, such as how the two MIDI IN connectors will function.
        case 0x00:
        {
            switch (Address)
            {
                case 0x00007F: // SYSTEM MODE SET
                    Description = msc::FormatText("System Parameter %06Xh \"System Mode Set %02Xh\" (%s)", Address, _Iter[3], (_Iter[3] == 0) ? "Mode 1" : "Unknown Mode"); break;

                default:
                    if (msc::InRange(Address, 0x000100u, 0x00011Fu)) // CHANNEL MSG RX PORT
                        Description = msc::FormatText("System Parameter %06Xh \"Channel Message Receive Port %02Xh\"", Address, _Iter[3]);
                    else
                        Description = msc::FormatText("System Parameter %06Xh \"Unknown\"", Address);
            }
            break;
        }

        // Temporary performance.
        case 0x01:
        {
            Description = msc::FormatText("Temporary performance. Unknown address %06Xh", Address);
            break;
        }

        // Performance mode.
        case 0x02:
        {
            Description = msc::FormatText("Performance mode. Unknown address %06Xh", Address);
            break;
        }

        // Patch mode.
        case 0x03:
        {
            Description = msc::FormatText("Patch mode. Unknown address %06Xh", Address);
            break;
        }

        // Display data.
        case 0x10:
        {
            if (msc::InRange(Address, 0x100000u, 0x10001Fu))
                Description = msc::FormatText("Display data %06Xh. Letter '%c'", Address, (char) _Iter[3]);
            else
            if (msc::InRange(Address, 0x100100u, 0x10013Fu))
                Description = msc::FormatText("Display data %06Xh. Dot data %02Xh", Address, _Iter[3]);
            else
                Description = msc::FormatText("Display data %06Xh. Unknown address", Address);
            break;
        }

        // User instrument.
        case 0x20:
        {
            Description = msc::FormatText("User instrument %06Xh", Address);
            break;
        }

        // User drum set.
        case 0x21:
        {
            Description = msc::FormatText("User drum set %06Xh", Address);
            break;
        }

        // Patch Common Parameters A. Parameters common to all Parts in each module (Block A 00-0F)
        case 0x40:
        // Patch Common Parameters B. Parameters common to all Parts in each module (Block B 10-1F)
        case 0x50:
        {
            Address = (Address & 0x00FFFF) | 0x400000; // Threat Patch Common Parameters B the same as Patch Common Parameters A.

            switch (Address)
            {
                case 0x400000: Description = msc::FormatText("Master Tune %02Xh (%d cents)",               _Iter[3], (int) _Iter[3]); break;

                case 0x400004: Description = msc::FormatText("Master Volume %02Xh (%d)",                   _Iter[3], (int) _Iter[3]); break;
                case 0x400006: Description = msc::FormatText("Master Key Shift %02Xh (%d semitones)",      _Iter[3], (int) _Iter[3]); break;
                case 0x400005: Description = msc::FormatText("Master Pan %02Xh (%d)",                      _Iter[3], (int) _Iter[3]); break;

                case 0x40007F: Description = msc::FormatText("Mode Set %02Xh (%s)",                        _Iter[3], "GS Reset"); break;

                case 0x400100:
                {
                    Description = "Patch Name ";

                    _Iter += 3;

                    while (*_Iter != 0xF7)
                    {
                        Description.append(1, (char) *_Iter++);
                    }
                    break;
                }

                case 0x400110: Description = "Reserved"; break;

                case 0x400130: Description = msc::FormatText("Reverb Macro %02Xh (%s)",                    _Iter[3], IdentifyGSReverbMacro(_Iter[3])); break;
                case 0x400131: Description = msc::FormatText("Reverb Character %02Xh (%s)",                _Iter[3], IdentifyGSReverbMacro(_Iter[3])); break;
                case 0x400132: Description = msc::FormatText("Reverb Pre-LPF %02Xh (%d)",                  _Iter[3], (int) _Iter[3]); break;
                case 0x400133: Description = msc::FormatText("Reverb Level %02Xh (%d)",                    _Iter[3], (int) _Iter[3]); break;
                case 0x400134: Description = msc::FormatText("Reverb Time %02Xh (%d)",                     _Iter[3], (int) _Iter[3]); break;
                case 0x400135: Description = msc::FormatText("Reverb Delay Feedback %02Xh (%d)",           _Iter[3], (int) _Iter[3]); break;
                case 0x400136: Description = msc::FormatText("Reverb Send Level to Chorus %02Xh (%d)",     _Iter[3], (int) _Iter[3]); break;
                case 0x400137: Description = msc::FormatText("Reverb Predelay Time %02Xh (%d ms)",         _Iter[3], (int) _Iter[3]); break;

                case 0x400138: Description = msc::FormatText("Chorus Macro %02Xh (%s)",                    _Iter[3], IdentifyGSChorusMacro(_Iter[3])); break;
                case 0x400139: Description = msc::FormatText("Chorus Pre-LPF %02Xh (%d)",                  _Iter[3], (int) _Iter[3]); break;
                case 0x40013A: Description = msc::FormatText("Chorus Level %02Xh (%d)",                    _Iter[3], (int) _Iter[3]); break;
                case 0x40013B: Description = msc::FormatText("Chorus Feedback %02Xh (%d)",                 _Iter[3], (int) _Iter[3]); break;
                case 0x40013C: Description = msc::FormatText("Chorus Delay %02Xh (%d)",                    _Iter[3], (int) _Iter[3]); break;
                case 0x40013D: Description = msc::FormatText("Chorus Rate %02Xh (%d))",                    _Iter[3], (int) _Iter[3]); break;
                case 0x40013E: Description = msc::FormatText("Chorus Depth %02Xh (%d)",                    _Iter[3], (int) _Iter[3]); break;
                case 0x40013F: Description = msc::FormatText("Chorus Send Level to Reverb %02Xh (%d)",     _Iter[3], (int) _Iter[3]); break;
                case 0x400140: Description = msc::FormatText("Chorus Send Level to Delay %02Xh (%d)",      _Iter[3], (int) _Iter[3]); break;

                case 0x400150: Description = msc::FormatText("Delay Macro %02Xh (%s)",                     _Iter[3], IdentifyGSDelayMacro(_Iter[3])); break;
                case 0x400151: Description = msc::FormatText("Delay Pre-LPF %02Xh (%d)",                   _Iter[3], (int) _Iter[3]); break;
                case 0x400152: Description = msc::FormatText("Delay Time Center %02Xh (%.1fms)",           _Iter[3], (float) _Iter[3] / 0x73); break;
                case 0x400153: Description = msc::FormatText("Delay Time Ratio Left %02Xh (%d%%)",         _Iter[3], (int) _Iter[3] * 4); break;
                case 0x400154: Description = msc::FormatText("Delay Time Ration Right %02Xh (%d%%)",       _Iter[3], (int) _Iter[3] * 4); break;
                case 0x400155: Description = msc::FormatText("Delay Level Center %02Xh (%d)",              _Iter[3], (int) _Iter[3]); break;
                case 0x400156: Description = msc::FormatText("Delay Level Left %02Xh (%d)",                _Iter[3], (int) _Iter[3]); break;
                case 0x400157: Description = msc::FormatText("Delay Level Right %02Xh (%d)",               _Iter[3], (int) _Iter[3]); break;
                case 0x400158: Description = msc::FormatText("Delay Level %02Xh (%d)",                     _Iter[3], (int) _Iter[3]); break;
                case 0x400159: Description = msc::FormatText("Delay Feedback %02Xh (%d)",                  _Iter[3], (int) _Iter[3]); break;
                case 0x40015A: Description = msc::FormatText("Delay Send Level to Reverb %02Xh (%d)",      _Iter[3], (int) _Iter[3]); break;

                case 0x400200: Description = msc::FormatText("Eq Low Frequency %02Xh (%dHz)",              _Iter[3], (_Iter[3] == 0 ? 200 : 400)); break;
                case 0x400201: Description = msc::FormatText("Eq Low Gain %02Xh (%ddB)",                   _Iter[3], Map((int) _Iter[3], 0x34, 0x4C, -12, 12)); break; // -12dB .. +12dB
                case 0x400202: Description = msc::FormatText("Eq High Frequency %02Xh (%dkHz)",            _Iter[3], (_Iter[3] == 0 ? 3 : 6)); break;
                case 0x400203: Description = msc::FormatText("Eq High Gain %02Xh (%ddB)",                  _Iter[3], Map((int) _Iter[3], 0x34, 0x4C, -12, 12)); break; // -12dB .. +12dB

                case 0x400300: Description = msc::FormatText("EFX Type %02Xh (%ddB)",                      _Iter[3], (int) _Iter[3]); break;

                case 0x400303: case 0x400304: case 0x400305: case 0x400306: case 0x400307: case 0x400308: case 0x400309: case 0x40030A: case 0x40030B: case 0x40030C:
                case 0x40030D: case 0x40030E: case 0x40030F: case 0x400310: case 0x400311: case 0x400312: case 0x400313: case 0x400314: case 0x400315: case 0x400316:
                               Description = msc::FormatText("EFX Parameter %d %02Xh (%d)", (int) _Iter[2] - 2, _Iter[3], (int) _Iter[3]); break;

                case 0x400317: Description = msc::FormatText("EFX Send Level to Reverb %02Xh (%d)",        _Iter[3], (int) _Iter[3]); break;
                case 0x400318: Description = msc::FormatText("EFX Send Level to Chorus %02Xh (%d)",        _Iter[3], (int) _Iter[3]); break;
                case 0x400319: Description = msc::FormatText("EFX Send Level to Delay %02Xh (%d)",         _Iter[3], (int) _Iter[3]); break;

                case 0x40031B: Description = msc::FormatText("EFX Control Source 1 %02Xh (%d)",            _Iter[3], (int) _Iter[3]); break;
                case 0x40031C: Description = msc::FormatText("EFX Control Depth 1 %02Xh (%d%%)",           _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100, 100)); break;
                case 0x40031D: Description = msc::FormatText("EFX Control Source 2 %02Xh (%d)",            _Iter[3], (int) _Iter[3]); break;
                case 0x40031E: Description = msc::FormatText("EFX Control Depth 2 %02Xh (%d%%)",           _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100, 100)); break;

                case 0x40031F: Description = msc::FormatText("EFX Send Level to Delay %02Xh (%s)",         _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;

                default:
                {
                    switch (Address & 0xFFF0FF)
                    {
                        // Patch Part Parameters. (Block A 00-0F)
                        case 0x401000: Description = msc::FormatText("Set Part %d to Tone Number %d (%02Xh)",              GSBlockToPart(_Iter[1]), (int) _Iter[3], _Iter[3]); break;

                        case 0x401002: Description = msc::FormatText("Rx. Channel %02Xh (%d)",                             _Iter[3], (int) _Iter[3]); break;
                        case 0x401003: Description = msc::FormatText("Rx. Pitch Bend %02Xh (%s)",                          _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401004: Description = msc::FormatText("Rx. Channel Pressure %02Xh (%s)",                    _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401005: Description = msc::FormatText("Rx. Program Change %02Xh (%s)",                      _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401006: Description = msc::FormatText("Rx. Control Change %02Xh (%s)",                      _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401007: Description = msc::FormatText("Rx. Poly Pressure %02Xh (%s)",                       _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401008: Description = msc::FormatText("Rx. Note Message %02Xh (%s)",                        _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401009: Description = msc::FormatText("Rx. RPN %02Xh (%s)",                                 _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100A: Description = msc::FormatText("Rx. NRPN %02Xh (%s)",                                _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100B: Description = msc::FormatText("Rx. Modulation %02Xh (%s)",                          _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100C: Description = msc::FormatText("Rx. Volume %02Xh (%s)",                              _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100D: Description = msc::FormatText("Rx. PanPot %02Xh (%s)",                              _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100E: Description = msc::FormatText("Rx. Expression %02Xh (%s)",                          _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x40100F: Description = msc::FormatText("Rx. Hold1 %02Xh (%s)",                               _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401010: Description = msc::FormatText("Rx. Portamento %02Xh (%s)",                          _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401011: Description = msc::FormatText("Rx. Sostenuto %02Xh (%s)",                           _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401012: Description = msc::FormatText("Rx. Soft %02Xh (%s)",                                _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;

                        case 0x401013: Description = msc::FormatText("Set Part %d to %s Mode",                             GSBlockToPart(_Iter[1]), (_Iter[3] == 0x01 ? "Poly" : "Mono")); break;
                        case 0x401014: Description = msc::FormatText("Assign Part %d Mode %s",                             GSBlockToPart(_Iter[1]), (_Iter[3] == 0x00 ? "Single" : (_Iter[3] == 0x01 ? "Limited-Multi" : "Full-Multi"))); break;
                        case 0x401015: Description = msc::FormatText("Use Part %d for Rhythm (%s)",                        GSBlockToPart(_Iter[1]), IdentifyGSRhythmPart(_Iter[3])); break;

                        case 0x401016: Description = msc::FormatText("Pitch Key Shift %02Xh (%d semitones)",               _Iter[3],           Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x401017: Description = msc::FormatText("Pitch Offset Fine %02Xh %02Xh (%.1fHz)",             _Iter[3], _Iter[4], Map((_Iter[4] << 7) | _Iter[3], 0x08, 0xF8, -12.f, 12.f)); break;

                        case 0x401019: Description = msc::FormatText("Set Part %d Volume Level %d (%02Xh)",                GSBlockToPart(_Iter[1]), (int) _Iter[3], _Iter[3]); break;
                        case 0x40101A: Description = msc::FormatText("Velocity Sense Depth %02Xh (%d)",                    _Iter[3], (int) _Iter[3]); break;
                        case 0x40101B: Description = msc::FormatText("Velocity Sense Offset %02Xh (%d)",                   _Iter[3], (int) _Iter[3]); break;
                        case 0x40101C: Description = msc::FormatText("Part PanPot %02Xh (%d)",                             _Iter[3], (int) _Iter[3]); break;
                        case 0x40101D: Description = msc::FormatText("Keyboard Range Low %02Xh (%d)",                      _Iter[3], (int) _Iter[3]); break;
                        case 0x40101E: Description = msc::FormatText("Keyboard Range High %02Xh (%d)",                     _Iter[3], (int) _Iter[3]); break;
                        case 0x40101F: Description = msc::FormatText("CC1 Controller NUmber %02Xh (%d)",                   _Iter[3], (int) _Iter[3]); break;
                        case 0x401020: Description = msc::FormatText("CC2 Controller Number %02Xh (%d)",                   _Iter[3], (int) _Iter[3]); break;
                        case 0x401021: Description = msc::FormatText("Chorus Send Level %02Xh (%d)",                       _Iter[3], (int) _Iter[3]); break;
                        case 0x401022: Description = msc::FormatText("Reverb Send Level %02Xh (%d)",                       _Iter[3], (int) _Iter[3]); break;

                        case 0x401023: Description = msc::FormatText("Rx. Bank Select %02Xh (%s)",                         _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x401024: Description = msc::FormatText("Rx. Bank Select LSB %02Xh (%s)",                     _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;

                        case 0x401025: Description = msc::FormatText("%06X <Undocumented>", Address); break;

                        case 0x40102A: Description = msc::FormatText("Pitch Fine Tune %02Xh %02Xh (%d cents)",             _Iter[3], _Iter[4], Map((_Iter[3] << 7) | _Iter[4], 0x0000, 0x7F7F, -100, 100)); break;

                        case 0x40102C: Description = msc::FormatText("Delay Send Level %02Xh (%d)",                        _Iter[3], (int) _Iter[3]); break;

                        case 0x401030: Description = msc::FormatText("Tone Modify 1 Vibrato Rate %02Xh (%d)",              _Iter[3], (int) _Iter[3]); break;
                        case 0x401031: Description = msc::FormatText("Tone Modify 2 Vibrato Depth %02Xh (%d)",             _Iter[3], (int) _Iter[3]); break;

                        case 0x401032: Description = msc::FormatText("Tone Modify 3 TVF Cutoff Freq. %02Xh (%d)",          _Iter[3], (int) _Iter[3]); break;
                        case 0x401033: Description = msc::FormatText("Tone Modify 4 TVF Resonance %02Xh (%d)",             _Iter[3], (int) _Iter[3]); break;
                        case 0x401034: Description = msc::FormatText("Tone Modify 5 TVF & TVA Env. attack %02Xh (%d)",     _Iter[3], (int) _Iter[3]); break;
                        case 0x401035: Description = msc::FormatText("Tone Modify 5 TVF & TVA Env. decay %02Xh (%d)",      _Iter[3], (int) _Iter[3]); break;
                        case 0x401036: Description = msc::FormatText("Tone Modify 5 TVF & TVA Env. release %02Xh (%d)",    _Iter[3], (int) _Iter[3]); break;

                        case 0x401037: Description = msc::FormatText("Tone Modify 8 Vibrato Delay %02Xh (%d)",             _Iter[3], (int) _Iter[3]); break;

                        case 0x401040: Description = msc::FormatText("Scale Tuning C %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401041: Description = msc::FormatText("Scale Tuning C# %02Xh (%d cents)",                   _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401042: Description = msc::FormatText("Scale Tuning D %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401043: Description = msc::FormatText("Scale Tuning D# %02Xh (%d cents)",                   _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401044: Description = msc::FormatText("Scale Tuning E %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401045: Description = msc::FormatText("Scale Tuning F %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401046: Description = msc::FormatText("Scale Tuning F# %02Xh (%d cents)",                   _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401047: Description = msc::FormatText("Scale Tuning G %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401048: Description = msc::FormatText("Scale Tuning G# %02Xh (%d cents)",                   _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x401049: Description = msc::FormatText("Scale Tuning A %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x40104A: Description = msc::FormatText("Scale Tuning A# %02Xh (%d cents)",                   _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes
                        case 0x40104B: Description = msc::FormatText("Scale Tuning B %02Xh (%d cents)",                    _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -64, 63)); break; // 12 bytes

                        case 0x402000: Description = msc::FormatText("Mod Pitch Control %02Xh (%d semitones)",             _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402001: Description = msc::FormatText("Mod TVF Cutoff Control %02Xh (%d cents)",            _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402002: Description = msc::FormatText("Mod Amplitude Control %02Xh (%.1f%%)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402003: Description = msc::FormatText("Mod LFO1 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402004: Description = msc::FormatText("Mod LFO1 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402005: Description = msc::FormatText("Mod LFO1 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402006: Description = msc::FormatText("Mod LFO1 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402007: Description = msc::FormatText("Mod LFO2 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402008: Description = msc::FormatText("Mod LFO2 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402009: Description = msc::FormatText("Mod LFO2 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40200A: Description = msc::FormatText("Mod LFO2 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402010: Description = msc::FormatText("Bend Pitch Control %02Xh (%d semitones)",            _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402011: Description = msc::FormatText("Bend TVF Cutoff Control %02Xh (%d cents)",           _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402012: Description = msc::FormatText("Bend Amplitude Control %02Xh (%.1f%%)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402013: Description = msc::FormatText("Bend LFO1 Rate Control %02Xh (%.1fHz)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402014: Description = msc::FormatText("Bend LFO1 Pitch Depth %02Xh (%d cents)",             _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402015: Description = msc::FormatText("Bend LFO1 TVF Depth %02Xh (%d cents)",               _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402016: Description = msc::FormatText("Bend LFO1 TVA Depth %02Xh (%.1f%%)",                 _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402017: Description = msc::FormatText("Bend LFO2 Rate Control %02Xh (%.1fHz)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402018: Description = msc::FormatText("Bend LFO2 Pitch Depth %02Xh (%d cents)",             _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402019: Description = msc::FormatText("Bend LFO2 TVF Depth %02Xh (%d cents)",               _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40201A: Description = msc::FormatText("Bend LFO2 TVA Depth %02Xh (%.1f%%)",                 _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402020: Description = msc::FormatText("CAf Pitch Control %02Xh (%d semitones)",             _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402021: Description = msc::FormatText("CAf TVF Cutoff Control %02Xh (%d cents)",            _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402022: Description = msc::FormatText("CAf Amplitude Control %02Xh (%.1f%%)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402023: Description = msc::FormatText("CAf LFO1 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402024: Description = msc::FormatText("CAf LFO1 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402025: Description = msc::FormatText("CAf LFO1 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402026: Description = msc::FormatText("CAf LFO1 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402027: Description = msc::FormatText("CAf LFO2 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402028: Description = msc::FormatText("CAf LFO2 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402029: Description = msc::FormatText("CAf LFO2 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40202A: Description = msc::FormatText("CAf LFO2 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402030: Description = msc::FormatText("PAf Pitch Control %02Xh (%d semitones)",             _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402031: Description = msc::FormatText("PAf TVF Cutoff Control %02Xh (%d cents)",            _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402032: Description = msc::FormatText("PAf Amplitude Control %02Xh (%.1f%%)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402033: Description = msc::FormatText("PAf LFO1 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402034: Description = msc::FormatText("PAf LFO1 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402035: Description = msc::FormatText("PAf LFO1 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402036: Description = msc::FormatText("PAf LFO1 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402037: Description = msc::FormatText("PAf LFO2 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402038: Description = msc::FormatText("PAf LFO2 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402039: Description = msc::FormatText("PAf LFO2 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40203A: Description = msc::FormatText("PAf LFO2 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402040: Description = msc::FormatText("CC1 Pitch Control %02Xh (%d semitones)",             _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402041: Description = msc::FormatText("CC1 TVF Cutoff Control %02Xh (%d cents)",            _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402042: Description = msc::FormatText("CC1 Amplitude Control %02Xh (%.1f%%)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402043: Description = msc::FormatText("CC1 LFO1 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402044: Description = msc::FormatText("CC1 LFO1 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402045: Description = msc::FormatText("CC1 LFO1 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402046: Description = msc::FormatText("CC1 LFO1 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402047: Description = msc::FormatText("CC1 LFO2 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402048: Description = msc::FormatText("CC1 LFO2 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402049: Description = msc::FormatText("CC1 LFO2 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40204A: Description = msc::FormatText("CC1 LFO2 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402050: Description = msc::FormatText("CC2 Pitch Control %02Xh (%d semitones)",             _Iter[3], Map((int) _Iter[3], 0x28, 0x58, -24, 24)); break;
                        case 0x402051: Description = msc::FormatText("CC2 TVF Cutoff Control %02Xh (%d cents)",            _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -9600, 9600)); break;
                        case 0x402052: Description = msc::FormatText("CC2 Amplitude Control %02Xh (%.1f%%)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -100.f, 100.f)); break;

                        case 0x402053: Description = msc::FormatText("CC2 LFO1 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402054: Description = msc::FormatText("CC2 LFO1 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402055: Description = msc::FormatText("CC2 LFO1 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x402056: Description = msc::FormatText("CC2 LFO1 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x402057: Description = msc::FormatText("CC2 LFO2 Rate Control %02Xh (%.1fHz)",               _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, -10.f, 10.f)); break;
                        case 0x402058: Description = msc::FormatText("CC2 LFO2 Pitch Depth %02Xh (%d cents)",              _Iter[3], Map((int) _Iter[3], 0x00, 0x7F, 0, 600)); break;
                        case 0x402059: Description = msc::FormatText("CC2 LFO2 TVF Depth %02Xh (%d cents)",                _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0, 2400)); break;
                        case 0x40205A: Description = msc::FormatText("CC2 LFO2 TVA Depth %02Xh (%.1f%%)",                  _Iter[3], Map((int) _Iter[3], 0x28, 0x58, 0.f, 100.f)); break;

                        case 0x404000: Description = msc::FormatText("Tone Map Number %d (%s)",                            _Iter[3], IdentifyGSToneMap(_Iter[3])); break;
                        case 0x404001: Description = msc::FormatText("Tone Map-0 Number %d (%s)",                          _Iter[3], IdentifyGSToneMap(_Iter[3])); break;

                        case 0x404020: Description = msc::FormatText("Eq %02Xh (%s)",                                      _Iter[3], (_Iter[3] == 0x01 ? "On" : "Off")); break;
                        case 0x404021: Description = msc::FormatText("Output Assign %02Xh (%d)",                           _Iter[3], (int) _Iter[3]); break;
                        case 0x404022: Description = msc::FormatText("Part EFX Assign %02Xh (%s)",                         _Iter[3], (_Iter[3] == 0x00 ? "Bypass" : "EFX")); break;

                        case 0x404023: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404024: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404025: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404026: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404027: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404028: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x404029: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402A: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402B: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402C: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402D: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402E: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                        case 0x40402F: Description = msc::FormatText("%06X <Undocumented>", Address); break;
                    }
                }
            }
            break;
        }

        // Drum Setup Parameters
        case 0x41:
        {
            if (Address < 0x420000)
            {
                if ((_Iter[1] == 0x00 || _Iter[1] == 0x10) && (_Iter[2] < 0x0C))
                    Description = msc::FormatText("Drum Map Name");
                else
                if ((_Iter[1] == 0x01 || _Iter[1] == 0x11))
                    Description = msc::FormatText("Drum Play Note Number");
                else
                if ((_Iter[1] == 0x02 || _Iter[1] == 0x12))
                    Description = msc::FormatText("Drum Level");
                else
                if ((_Iter[1] == 0x03 || _Iter[1] == 0x13))
                    Description = msc::FormatText("Drum Assign Group Number");
                else
                if ((_Iter[1] == 0x04 || _Iter[1] == 0x14))
                    Description = msc::FormatText("Drum Pan Pot");
                else
                if ((_Iter[1] == 0x05 || _Iter[1] == 0x15))
                    Description = msc::FormatText("Drum Reverb Send Level");
                else
                if ((_Iter[1] == 0x06 || _Iter[1] == 0x16))
                    Description = msc::FormatText("Drum Chorus Send Level");
                else
                if ((_Iter[1] == 0x07 || _Iter[1] == 0x17))
                    Description = msc::FormatText("Drum Rx. Note Off");
                else
                if ((_Iter[1] == 0x08 || _Iter[1] == 0x18))
                    Description = msc::FormatText("Drum Rx. Note On");
                else
                if ((_Iter[1] == 0x09 || _Iter[1] == 0x19))
                    Description = msc::FormatText("Drum Delay Send Level");
            }
            break;
        }

        default:
            Description = msc::FormatText("Unknown address %06Xh", Address);
    }

    IsChecksumValid = (_Data[_Data.size() - 2] != CalculateRolandCheckSum(_Data.data(), _Data.size()));
}

/// <summary>
/// Converts a block number to a part number.
/// </summary>
int sysex_t::GSBlockToPart(int value) noexcept
{
    value &= 0x0F;

    if (value == 0)
        return 10;

    if (value < 10)
        return value;

    return value + 1;
}

const char * sysex_t::IdentifyGSReverbMacro(uint8_t value) noexcept
{
    switch (value)
    {
        case 0x00: return "Room 1"; break;
        case 0x01: return "Room 2"; break;
        case 0x02: return "Room 3"; break;
        case 0x03: return "Hall 1"; break;
        case 0x04: return "Hall 2"; break;
        case 0x05: return "Plate"; break;
        case 0x06: return "Delay"; break;
        case 0x07: return "Panning Delay"; break;

        default: return "<Unknown>";
    }
}

const char * sysex_t::IdentifyGSChorusMacro(uint8_t value) noexcept
{
    switch (value)
    {
        case 0x00: return "Chorus 1"; break;
        case 0x01: return "Chorus 2"; break;
        case 0x02: return "Chorus 3"; break;
        case 0x03: return "Chorus 4"; break;
        case 0x04: return "Feedback Chorus"; break;
        case 0x05: return "Flanger"; break;
        case 0x06: return "Short Delay"; break;
        case 0x07: return "Short Delay (FB)"; break;

        default: return "<Unknown>";
    }
}

const char * sysex_t::IdentifyGSDelayMacro(uint8_t value) noexcept
{
    switch (value)
    {
        case 0x00: return "Delay 1"; break;
        case 0x01: return "Delay 2"; break;
        case 0x02: return "Delay 3"; break;
        case 0x03: return "Delay 4"; break;
        case 0x04: return "Pan Delay 1"; break;
        case 0x05: return "Pan Delay 2"; break;
        case 0x06: return "Pan Delay 3"; break;
        case 0x07: return "Pan Delay 4"; break;
        case 0x08: return "Delay to Reverb"; break;
        case 0x09: return "Pan Repeat"; break;

        default: return "<Unknown>";
    }
}

const char * sysex_t::IdentifyGSRhythmPart(uint8_t value) noexcept
{
    switch (value)
    {
        case 0x00: return "Off"; break;
        case 0x01: return "Map 1"; break;
        case 0x02: return "Map 2"; break;

        default: return "<Unknown>";
    }
}

const char * sysex_t::IdentifyGSToneMap(uint8_t value) noexcept
{
    switch (value)
    {
        case 0x00: return "Selected"; break;
        case 0x01: return "SC-55 Map"; break;
        case 0x02: return "SC-88 Map"; break;
        case 0x03: return "SC-88Pro Map"; break;
        case 0x04: return "SC-8850 Map"; break;

        default: return "<Unknown>";
    }
}

/// <summary>
/// Identifies a Yamaha SysEx.
/// </summary>
void sysex_t::IdentifyYamaha() noexcept
{
    Manufacturer = "Yamaha";

    _Iter = _Data.begin() + 3;
/*
    // Yamaha, Device 0x79
    { "\xF0\x43" "\x00" "\x05\x7E\x09",                                     6, L"SMW-5 Volume" },           // MMF/SMAF MMFTool
    { "\xF0\x43" "\x00" "\x06\x7F\x04",                                     6, L"SMW-5 Detach Wave" },      // MMF/SMAF MMFTool
    { "\xF0\x43" "\x00" "\x06\x7F\x7F\xF7",                                 7, L"SMW-5 Reset" },            // MMF/SMAF MMFTool
*/
    Model = "Unknown";
    Description = "Unknown";

    switch (*_Iter++)
    {
        case 0x1A:
        {
            Model = "S-YXG2006LE / S-YXG50 Soft Synth";

            const uint8_t Data[] =  { 0x10, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xF7 };

            if ((_Data.size() == 14) && ::memcmp(&_Iter[0], Data, _countof(Data)) == 0)
                Description = "XG Works On";
            else
            {
                uint32_t Address = (uint32_t) (_Iter[0] << 8) | _Iter[1];

                switch (Address)
                {
                    // Engine Initialization / State Control: Sent by Yamaha’s driver before any XG data is transmitted.
                    case 0x0000: Description = "Disable synth engine"; break;
                    case 0x0001: Description = "Enable synth engine"; break;
                    case 0x0002: Description = "Reinitialize internal buffers"; break;
                    case 0x007F: Description = "Full internal reset"; break;

                    // Audio Rendering Mode: These modes affect CPU usage and quality. Offline mode is used by some DAWs when exporting audio.
                    case 0x1000: Description = "Switch to real-time rendering"; break;
                    case 0x1001: Description = "Switch to high-precision offline rendering"; break;
                    case 0x1002: Description = "Switch to low-latency mode"; break;

                    // Effect Engine Control: These are not the same as XG effect parameter changes — they control the DSP engine itself.
                    case 0x2000: Description = "Disable all effects"; break;
                    case 0x2001: Description = "Enable effects engine"; break;
                    case 0x2002: Description = "Reinitialize effect DSP"; break;

                    case 0x2100: Description = "Set effect processing quality to Low"; break;
                    case 0x2101: Description = "Set effect processing quality to Normal"; break;
                    case 0x2102: Description = "Set effect processing quality to High"; break;

                    // Voice Engine / Polyphony Control: The S‑YXG2006LE can exceed the hardware XG modules’ polyphony, so this matters.
                    case 0x3000: Description = "Limit polyphony to low mode"; break;
                    case 0x3001: Description = "Normal polyphony"; break;
                    case 0x3002: Description = "High polyphony (CPU-heavy)"; break;

                    case 0x3100: Description = "Set max. polyphony to xx voices"; break;

                    // Internal Debug / Diagnostic Commands
                    case 0x4000: Description = "Query engine status"; break;
                    case 0x4001: Description = "Query DSP load"; break;
                    case 0x4002: Description = "Query voice allocation table"; break;

                    case 0x4100: Description = "Enable/disable debug logging"; break;

                    // Hidden XG Compatibility Controls: These override the engine mode without sending the usual GM/XG/GS reset sequences.
                    case 0x5000: Description = "Force GM mode (not GM reset)"; break;
                    case 0x5001: Description = "Force XG mode (not XG reset)"; break;
                    case 0x5002: Description = "Force TG300B mode"; break;

                    // Sample / Wave ROM Handling: The S‑YXG2006LE has multiple internal ROM banks for different quality levels.
                    case 0x6000: Description = "Reload wave ROM"; break;
                    case 0x6001: Description = "Flush sample cache"; break;
                    case 0x6002: Description = "Switch to alternate ROM set (if available)"; break;

                    // Licensing / DRM (Observed but not recommended): Should not be used. Used by the installer and VST wrapper.
                    case 0x7000: Description = "Query license state"; break;
                    case 0x7001: Description = "Trigger license refresh"; break;
                    case 0x707F: Description = "Force license check"; break;
                }
            }

            break;
        }

        case 0x49:
        {
            Model = "MU80/MU128";

            uint32_t Address = (uint32_t) (_Iter[0] << 16) | (_Iter[1] << 8) | _Iter[2];

            Description = msc::FormatText("Unknown address %06Xh", Address);
            break;
        }

        case 0x59:
        {
            Model = "MU90";

            uint32_t Address = (uint32_t) (_Iter[0] << 16) | (_Iter[1] << 8) | _Iter[2];

            Description = msc::FormatText("Unknown address %06Xh", Address);
            break;
        }

        case 0x7A: Model = "SW60XG"; break;

        // Yamaha, XG
        case 0x4C:
        {
            Model = "XG";

            uint32_t Address = (uint32_t) (_Iter[0] << 16) | (_Iter[1] << 8) | _Iter[2];

            if (_Data.size() >= 7)
            {
                switch (Address)
                {
                    case 0x000000: Description = "Master Tune"; break;
                    case 0x000004: Description = "Master Volume"; break;
                    case 0x000005: Description = "Master Attenuator"; break;
                    case 0x000006: Description = "Master Transpose"; break;

                    case 0x00007D:
                    {
                        switch ((_Iter[3] << 8) << _Iter[4])
                        {
                            case 0x01F7: Description = "Drum Setup 1 Reset"; break;
                            case 0x02F7: Description = "Drum Setup 2 Reset"; break;
                        }
                        break;
                    }

                    case 0x00007E: if (((_Iter[3] << 8) | _Iter[4]) == 0x00F7) Description = "System On"; break;
                    case 0x00007F: if (((_Iter[3] << 8) | _Iter[4]) == 0x00F7) Description = "All Parameters Reset"; break;

                    case 0x020100: Description = "Reverb Type"; break;
                    case 0x020102: Description = "Reverb Parameter 1"; break;
                    case 0x020103: Description = "Reverb Parameter 2"; break;
                    case 0x020104: Description = "Reverb Parameter 3"; break;
                    case 0x020105: Description = "Reverb Parameter 4"; break;
                    case 0x020106: Description = "Reverb Parameter 5"; break;
                    case 0x020107: Description = "Reverb Parameter 6"; break;
                    case 0x020108: Description = "Reverb Parameter 7"; break;
                    case 0x020109: Description = "Reverb Parameter 8"; break;
                    case 0x02010A: Description = "Reverb Parameter 9"; break;
                    case 0x02010B: Description = "Reverb Parameter 10"; break;
                    case 0x02010C: Description = "Reverb Return"; break;
                    case 0x02010D: Description = "Reverb Pan"; break;

                    case 0x020110: Description = "Reverb Parameter 11"; break;
                    case 0x020111: Description = "Reverb Parameter 12"; break;
                    case 0x020112: Description = "Reverb Parameter 13"; break;
                    case 0x020113: Description = "Reverb Parameter 14"; break;
                    case 0x020114: Description = "Reverb Parameter 15"; break;
                    case 0x020115: Description = "Reverb Parameter 16"; break;

                    case 0x020120: Description = "Chorus Type"; break;
                    case 0x020122: Description = "Chorus Parameter 1"; break;
                    case 0x020123: Description = "Chorus Parameter 2"; break;
                    case 0x020124: Description = "Chorus Parameter 3"; break;
                    case 0x020125: Description = "Chorus Parameter 4"; break;
                    case 0x020126: Description = "Chorus Parameter 5"; break;
                    case 0x020127: Description = "Chorus Parameter 6"; break;
                    case 0x020128: Description = "Chorus Parameter 7"; break;
                    case 0x020129: Description = "Chorus Parameter 8"; break;
                    case 0x02012A: Description = "Chorus Parameter 9"; break;
                    case 0x02012B: Description = "Chorus Parameter 10"; break;
                    case 0x02012C: Description = "Chorus Return"; break;
                    case 0x02012D: Description = "Chorus Pan"; break;
                    case 0x02012E: Description = "Send Chorus to Reverb"; break;

                    case 0x020130: Description = "Chorus Parameter 11"; break;
                    case 0x020131: Description = "Chorus Parameter 12"; break;
                    case 0x020132: Description = "Chorus Parameter 13"; break;
                    case 0x020133: Description = "Chorus Parameter 14"; break;
                    case 0x020134: Description = "Chorus Parameter 15"; break;
                    case 0x020135: Description = "Chorus Parameter 16"; break;

                    case 0x020140: Description = "Variation Type"; break;
                    case 0x020142: Description = "Variation Parameter 1"; break;
                    case 0x020144: Description = "Variation Parameter 2"; break;
                    case 0x020146: Description = "Variation Parameter 3"; break;
                    case 0x020148: Description = "Variation Parameter 4"; break;
                    case 0x02014A: Description = "Variation Parameter 5"; break;
                    case 0x02014C: Description = "Variation Parameter 6"; break;
                    case 0x02014E: Description = "Variation Parameter 7"; break;
                    case 0x020150: Description = "Variation Parameter 8"; break;
                    case 0x020152: Description = "Variation Parameter 9"; break;
                    case 0x020154: Description = "Variation Parameter 10"; break;
                    case 0x020156: Description = "Variation Return"; break;
                    case 0x020157: Description = "Variation Pan"; break;
                    case 0x020158: Description = "Send Variation to Reverb"; break;
                    case 0x020159: Description = "Send Variation to Chorus"; break;
                    case 0x02015A: Description = "Variation Connection"; break;
                    case 0x02015B: Description = "Variation Part Number"; break;
                    case 0x02015C: Description = "MW Variation Control Depth"; break;
                    case 0x02015D: Description = "Bend Variation Control Depth"; break;
                    case 0x02015E: Description = "CAT Variation Control Depth"; break;
                    case 0x02015F: Description = "AC1 Variation Control Depth"; break;

                    case 0x020160: Description = "AC2 Variation Control Depth"; break;

                    case 0x020170: Description = "Variation Parameter 11"; break;
                    case 0x020171: Description = "Variation Parameter 12"; break;
                    case 0x020172: Description = "Variation Parameter 13"; break;
                    case 0x020173: Description = "Variation Parameter 14"; break;
                    case 0x020174: Description = "Variation Parameter 15"; break;
                    case 0x020175: Description = "Variation Parameter 16"; break;

                    case 0x024000: Description = "EQ Type"; break;

                    case 0x024001: Description = "EQ Gain 1"; break;
                    case 0x024002: Description = "EQ Frequency 1"; break;
                    case 0x024003: Description = "EQ Q1"; break;
                    case 0x024004: Description = "EQ Shape 1"; break;

                    case 0x024005: Description = "EQ Gain 2"; break;
                    case 0x024006: Description = "EQ Frequency 2"; break;
                    case 0x024007: Description = "EQ Q2"; break;
                    case 0x024008: Description = "Unused"; break;

                    case 0x024009: Description = "EQ Gain 3"; break;
                    case 0x02400A: Description = "EQ Frequency 3"; break;
                    case 0x02400B: Description = "EQ Q3"; break;
                    case 0x02400C: Description = "Unused"; break;

                    case 0x02400D: Description = "EQ Gain 4"; break;
                    case 0x02400E: Description = "EQ Frequency 4"; break;
                    case 0x02400F: Description = "EQ Q4"; break;
                    case 0x024010: Description = "Unused"; break;

                    case 0x024011: Description = "EQ Gain 5"; break;
                    case 0x024012: Description = "EQ Frequency 5"; break;
                    case 0x024013: Description = "EQ Q5"; break;
                    case 0x024014: Description = "EQ Shape 5"; break;

                    case 0x024020: Description = "EQ Gain 6"; break;
                    case 0x024021: Description = "EQ Frequency 6"; break;
                    case 0x024022: Description = "EQ Q6"; break;
                    case 0x024023: Description = "Unused"; break;

                    case 0x024024: Description = "EQ Gain 7"; break;
                    case 0x024025: Description = "EQ Frequency 7"; break;
                    case 0x024026: Description = "EQ Q7"; break;
                    case 0x024027: Description = "Unused"; break;

                    case 0x024028: Description = "EQ Gain 8"; break;
                    case 0x024029: Description = "EQ Frequency 8"; break;
                    case 0x02402A: Description = "EQ Q8"; break;
                    case 0x02402B: Description = "Unused"; break;

                    case 0x02402C: Description = "EQ Gain 9"; break;
                    case 0x02402D: Description = "EQ Frequency 9"; break;
                    case 0x02402E: Description = "EQ Q9"; break;
                    case 0x02402F: Description = "Unused"; break;

                    case 0x024030: Description = "EQ Gain 10"; break;
                    case 0x024031: Description = "EQ Frequency 10"; break;
                    case 0x024032: Description = "EQ Q10"; break;
                    case 0x024033: Description = "Unused"; break;

                    case 0x030000: Description = "Insertion Effect 1 Type"; break;

                    case 0x030100: Description = "Insertion Effect 2 Type"; break;

                    case 0x030200: Description = "Insertion Effect 3 Type"; break;

                    case 0x030300: Description = "Insertion Effect 4 Type"; break;

                    case 0x040000: Description = "Vocal Harmony Type"; break;

                    case 0x040002: Description = "Vocal Harmony Parameter 1"; break;
                    case 0x040003: Description = "Vocal Harmony Parameter 2"; break;
                    case 0x040004: Description = "Vocal Harmony Parameter 3"; break;
                    case 0x040005: Description = "Vocal Harmony Parameter 4"; break;
                    case 0x040006: Description = "Vocal Harmony Parameter 5"; break;
                    case 0x040007: Description = "Vocal Harmony Parameter 6"; break;
                    case 0x040008: Description = "Vocal Harmony Parameter 7"; break;
                    case 0x040009: Description = "Vocal Harmony Parameter 8"; break;
                    case 0x04000A: Description = "Vocal Harmony Parameter 9"; break;
                    case 0x04000B: Description = "Vocal Harmony Parameter 10"; break;
                    case 0x04000C: Description = "Vocal Harmony Part Number"; break;
                    case 0x04000D: Description = "MW Insertion Control Depth"; break;
                    case 0x04000E: Description = "Bend Insertion Control Depth"; break;
                    case 0x04000F: Description = "CAT Insertion Control Depth"; break;
                    case 0x040010: Description = "AC1 Insertion Control Depth"; break;
                    case 0x040011: Description = "AC2 Insertion Control Depth"; break;

                    case 0x040014: Description = "Harmony Channel"; break;
                    case 0x040015: Description = "Melody Channel"; break;

                    case 0x040016: Description = "Lead Output Level"; break;
                    case 0x040017: Description = "Harmony Output Level"; break;

                    case 0x040018: Description = "Lead Vocal Effect Dry Level"; break;
                    case 0x040019: Description = "Harmony Vocal Effect Dry Level"; break;
                    case 0x04001A: Description = "Lead Vocal Effect Send Level"; break;
                    case 0x04001B: Description = "Harmony Vocal Effect Send Level"; break;

                    case 0x040020: Description = "Vocal Harmony Parameter 11"; break;
                    case 0x040021: Description = "Vocal Harmony Parameter 12"; break;
                    case 0x040022: Description = "Vocal Harmony Parameter 13"; break;
                    case 0x040023: Description = "Vocal Harmony Parameter 14"; break;
                    case 0x040024: Description = "Vocal Harmony Parameter 15"; break;
                    case 0x040025: Description = "Vocal Harmony Parameter 16"; break;

                    case 0x040026: Description = "Vocal Harmony Parameter 17"; break;
                    case 0x040027: Description = "Vocal Harmony Parameter 18"; break;
                    case 0x040028: Description = "Vocal Harmony Parameter 19"; break;
                    case 0x040029: Description = "Vocal Harmony Parameter 20"; break;
                    case 0x04002A: Description = "Vocal Harmony Parameter 21"; break;
                    case 0x04002B: Description = "Vocal Harmony Parameter 22"; break;
                    case 0x04002C: Description = "Vocal Harmony Parameter 23"; break;
                    case 0x04002D: Description = "Vocal Harmony Parameter 24"; break;
                    case 0x04002E: Description = "Vocal Harmony Parameter 25"; break;
                    case 0x04002F: Description = "Vocal Harmony Parameter 26"; break;
                    case 0x040030: Description = "Vocal Harmony Parameter 27"; break;
                    case 0x040031: Description = "Vocal Harmony Parameter 28"; break;
                    case 0x040032: Description = "Vocal Harmony Parameter 29"; break;
                    case 0x040033: Description = "Vocal Harmony Parameter 30"; break;
                    case 0x040034: Description = "Vocal Harmony Parameter 31"; break;
                    case 0x040035: Description = "Vocal Harmony Parameter 32"; break;
                    case 0x040036: Description = "Vocal Harmony Parameter 33"; break;
                    case 0x040037: Description = "Vocal Harmony Parameter 34"; break;
                    case 0x040038: Description = "Vocal Harmony Parameter 35"; break;
                    case 0x040039: Description = "Vocal Harmony Parameter 36"; break;
                    case 0x04003A: Description = "Vocal Harmony Parameter 37"; break;
                    case 0x04003B: Description = "Vocal Harmony Parameter 38"; break;
                    case 0x04003C: Description = "Vocal Harmony Parameter 39"; break;
                    case 0x04003D: Description = "Vocal Harmony Parameter 40"; break;
                    case 0x04003E: Description = "Vocal Harmony Parameter 41"; break;
                    case 0x04003F: Description = "Vocal Harmony Parameter 42"; break;
                    case 0x040040: Description = "Vocal Harmony Parameter 43"; break;
                    case 0x040041: Description = "Vocal Harmony Parameter 44"; break;
                    case 0x040042: Description = "Vocal Harmony Parameter 45"; break;
                    case 0x040043: Description = "Vocal Harmony Parameter 46"; break;
                    case 0x040044: Description = "Vocal Harmony Parameter 47"; break;
                    case 0x040045: Description = "Vocal Harmony Parameter 48"; break;

                    case 0x040060: Description = "Vocal Effect Type"; break;

                    case 0x060000: Description = "Display Letter"; break;
                    case 0x070000: Description = "Display Bit Map"; break;

                    default:
                    {
                        Address &= 0xFF00FF;

                        switch (Address)
                        {
                            case 0x080000: Description = "Element Reserve"; break;
                            case 0x080001: Description = "Bank Select MSB"; break;
                            case 0x080002: Description = "Bank Select LSB"; break;
                            case 0x080003: Description = "Program Number"; break;
                            case 0x080004: Description = "Rcv Channel"; break;
                            case 0x080005: Description = "Mono/Poly Mode"; break;
                            case 0x080006: Description = "Same Note Number Key On Assign"; break;
                            case 0x080007: Description = "Part Mode"; break;
                            case 0x080008: Description = "Note Shift"; break;
                            case 0x080009: Description = "Detune"; break;

                            case 0x08000B: Description = "Volume"; break;
                            case 0x08000C: Description = "Volume Sense Depth"; break;
                            case 0x08000D: Description = "Volume Sense Offset"; break;
                            case 0x08000E: Description = "Pan"; break;
                            case 0x08000F: Description = "Note Limit Low"; break;
                            case 0x080010: Description = "Note Limit High"; break;
                            case 0x080011: Description = "Dry Level"; break;
                            case 0x080012: Description = "Chorus Send"; break;
                            case 0x080013: Description = "Reverb Send"; break;
                            case 0x080014: Description = "Variation Send"; break;
                            case 0x080015: Description = "Vibrato Rate"; break;
                            case 0x080016: Description = "Vibrato Depth"; break;
                            case 0x080017: Description = "Vibrato Delay"; break;
                            case 0x080018: Description = "Filter Cutoff Frequency"; break;

                            default:
                                Description = msc::FormatText("Unknown address %06Xh", Address);
                        }
                    }
                }
            }
            else
            if (_Data.size() >= 6)
            {
                switch (Address)
                {
                    case 0x0800: Description = "Multi Part 1"; break;
                    case 0x0801: Description = "Multi Part 2"; break;
                    case 0x0802: Description = "Multi Part 3"; break;
                    case 0x0803: Description = "Multi Part 4"; break;
                    case 0x0804: Description = "Multi Part 5"; break;

                    case 0x300D: Description = "Drum Setup 1"; break;
                    case 0x310D: Description = "Drum Setup 2"; break;
                    case 0x320D: Description = "Drum Setup 3"; break;
                    case 0x330D: Description = "Drum Setup 4"; break;
                }
            }
            break;
        }

        case 0x7F:
        {
            switch (*_Iter)
            {
                case 0x17: Model = "MX88"; break;

                case 0x1C:
                {
                    if (*_Iter == 0x02)
                        Model = "Montage";

                    break;
                }
            }

            break;
        }
    }
}

}
