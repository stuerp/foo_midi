
/** $VER: Messages.cpp (2025.07.18) P. Stuer **/

#include "pch.h"

#include "Messages.h"

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T>
inline static T InRange(T value, T minValue, T maxValue)
{
    return (minValue <= value) && (value <= maxValue);
}

// General MIDI Level 1 Instrument Families
const std::vector<const char *> Instruments =
{
    //   1 -  8 Piano
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",

    //   9 - 16 Chromatic Percussion
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",

    //  17 - 24 Organ
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",

    //  25 - 32 Guitar
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar harmonics",

    //  33 - 40 Bass
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",

    //  41 - 48 Strings
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",

    //  49 - 56 Ensemble
    "String Ensemble 1",
    "String Ensemble 2",
    "SynthStrings 1",
    "SynthStrings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",

    //  57 - 64 Brass
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "SynthBrass 1",
    "SynthBrass 2",

    //  65 - 72 Reed
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",

    //  73 - 80 Pipe
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",

    //  81 - 88 Synth Lead
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",

    //  89 - 96 Synth Pad
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",

    //  97 - 104 Synth Effects
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",

    // 105 - 112 Ethnic
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",

    // 113 - 120 Percussive
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",

    // 121 - 128 Sound Effects
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",
};

// General MIDI Level 1 Percussion Key Map
const std::unordered_map<int, const char *> Percussions =
{
    { 35, "Acoustic Bass Drum" },
    { 36, "Bass Drum 1" },
    { 37, "Side Stick" },
    { 38, "Acoustic Snare" },
    { 39, "Hand Clap" },
    { 40, "Electric Snare" },
    { 41, "Low Floor Tom" },
    { 42, "Closed Hi Hat" },
    { 43, "High Floor Tom" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Low Tom" },
    { 46, "Open Hi-Hat" },
    { 47, "Low-Mid Tom" },
    { 48, "Hi-Mid Tom" },
    { 49, "Crash Cymbal 1" },
    { 50, "High Tom" },
    { 51, "Ride Cymbal 1" },
    { 52, "Chinese Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Tambourine" },
    { 55, "Splash Cymbal" },
    { 56, "Cowbell" },
    { 57, "Crash Cymbal 2" },
    { 58, "Vibraslap" },
    { 59, "Ride Cymbal 2" },
    { 60, "Hi Bongo" },
    { 61, "Low Bongo" },
    { 62, "Mute Hi Conga" },
    { 63, "Open Hi Conga" },
    { 64, "Low Conga" },
    { 65, "High Timbale" },
    { 66, "Low Timbale" },
    { 67, "High Agogo" },
    { 68, "Low Agogo" },
    { 69, "Cabasa" },
    { 70, "Maracas" },
    { 71, "Short Whistle" },
    { 72, "Long Whistle" },
    { 73, "Short Guiro" },
    { 74, "Long Guiro" },
    { 75, "Claves" },
    { 76, "Hi Wood Block" },
    { 77, "Low Wood Block" },
    { 78, "Mute Cuica" },
    { 79, "Open Cuica" },
    { 80, "Mute Triangle" },
    { 81, "Open Triangle" },
};

/// <summary>
/// Processes a Control Change message.
/// </summary>
std::string DescribeControlChange(uint8_t d1, uint8_t d2) noexcept
{
    std::string Line; Line.resize(256);

    int Value = (int) d2;

    switch (d1)
    {
        case 0x00: ::snprintf(Line.data(), Line.size(), "Bank Select (MSB) %d", Value); break;                                                     // Allows user to switch bank for patch selection. Program change used with Bank Select. MIDI can access 16,384 patches per MIDI channel
        case 0x01: ::snprintf(Line.data(), Line.size(), "Modulation Depth (MSB) %d", Value); break;                                                // Generally this CC controls a vibrato effect (pitch, loudness, brighness). What is modulated is based on the patch.
        case 0x02: ::snprintf(Line.data(), Line.size(), "Breath Controller (MSB) %d", Value); break;                                               // Often times associated with aftertouch messages. It was originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.

        case 0x03: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x04: ::snprintf(Line.data(), Line.size(), "Foot Controller (MSB) %d", Value); break;                                                 // Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
        case 0x05: ::snprintf(Line.data(), Line.size(), "Portamento Time (MSB) %d", Value); break;                                                 // Controls portamento rate to slide between 2 notes played subsequently. (GM2)
        case 0x06: ::snprintf(Line.data(), Line.size(), "Data Entry (MSB) %d", Value); break;                                                      // Sets the Value for NRPN or RPN parameters. (GM2)
        case 0x07: ::snprintf(Line.data(), Line.size(), "Channel Volume (MSB) %d", Value); break;                                                  // Control the volume of the channel
        case 0x08: ::snprintf(Line.data(), Line.size(), "Balance (MSB) %d", Value); break;                                                         // Controls the left and right balance, generally for stereo patches. 0 = hard left, 64 = center, 127 = hard right.

        case 0x09: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x0A: ::snprintf(Line.data(), Line.size(), "Pan (MSB) %d", Value); break;                                                             // Controls the left and right balance, generally for mono patches. 0 = hard left, 64 = center, 127 = hard right.
        case 0x0B: ::snprintf(Line.data(), Line.size(), "Expression (MSB) %d", Value); break;                                                      // Expression is a percentage of volume (CC7).
        case 0x0C: ::snprintf(Line.data(), Line.size(), "Effect Controller 1 (MSB) %d", Value); break;                                             // Usually used to control a parameter of an effect within the synth/workstation.
        case 0x0D: ::snprintf(Line.data(), Line.size(), "Effect Controller 2 (MSB) %d", Value); break;                                             // Usually used to control a parameter of an effect within the synth/workstation.

        case 0x0E: ::snprintf(Line.data(), Line.size(), "Undefined"); break;
        case 0x0F: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x10: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 1 (MSB)"); break;
        case 0x11: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 2 (MSB)"); break;
        case 0x12: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 3 (MSB)"); break;
        case 0x13: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 4 (MSB)"); break;

        // LSB
        case 0x20: ::snprintf(Line.data(), Line.size(), "Bank Select (LSB) %d", Value); break;                                                     // Allows user to switch bank for patch selection. Program change used with Bank Select. MIDI can access 16,384 patches per MIDI channel
        case 0x21: ::snprintf(Line.data(), Line.size(), "Modulation Depth (LSB) %d", Value); break;                                                // Generally this CC controls a vibrato effect (pitch, loudness, brighness). What is modulated is based on the patch.
        case 0x22: ::snprintf(Line.data(), Line.size(), "Breath Controller (LSB) %d", Value); break;                                               // Often times associated with aftertouch messages. It was originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.

        case 0x23: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x24: ::snprintf(Line.data(), Line.size(), "Foot Controller (LSB) %d", Value); break;                                                 // Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
        case 0x25: ::snprintf(Line.data(), Line.size(), "Portamento Time (LSB) %d", Value); break;                                                 // Controls portamento rate to slide between 2 notes played subsequently.
        case 0x26: ::snprintf(Line.data(), Line.size(), "Data Entry (LSB) %d", Value); break;                                                      // Sets the Value for NRPN or RPN parameters. (GM2)
        case 0x27: ::snprintf(Line.data(), Line.size(), "Channel Volume (LSB) %d", Value); break;                                                  // Control the volume of the channel
        case 0x28: ::snprintf(Line.data(), Line.size(), "Balance (LSB) %d", Value); break;                                                         // Controls the left and right balance, generally for stereo patches. 0 = hard left, 64 = center, 127 = hard right.

        case 0x29: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x2A: ::snprintf(Line.data(), Line.size(), "Pan (LSB) %d", Value); break;                                                             // Controls the left and right balance, generally for mono patches. 0 = hard left, 64 = center, 127 = hard right.
        case 0x2B: ::snprintf(Line.data(), Line.size(), "Expression (LSB) %d", Value); break;                                                      // Expression is a percentage of volume (CC7).
        case 0x2C: ::snprintf(Line.data(), Line.size(), "Effect Controller 1 (LSB) %d", Value); break;                                             // Usually used to control a parameter of an effect within the synth/workstation.
        case 0x2D: ::snprintf(Line.data(), Line.size(), "Effect Controller 2 (LSB) %d", Value); break;                                             // Usually used to control a parameter of an effect within the synth/workstation.

        case 0x2E: ::snprintf(Line.data(), Line.size(), "Undefined"); break;
        case 0x2F: ::snprintf(Line.data(), Line.size(), "Undefined"); break;

        case 0x30: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 1 (LSB)"); break;
        case 0x31: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 2 (LSB)"); break;
        case 0x32: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 3 (LSB)"); break;
        case 0x33: ::snprintf(Line.data(), Line.size(), "General Purpose Controller 4 (LSB)"); break;

        // ***
        case 0x40: ::snprintf(Line.data(), Line.size(), "Hold 1 (Damper Pedal / Sustain Pedal) %s", (Value < 64 ? "off" : "on")); break;            // On/Off switch that controls sustain. (See also Sostenuto CC 66) 0 to 63 = Off, 64 to 127 = On
        case 0x41: ::snprintf(Line.data(), Line.size(), "Portamento %s", (Value < 64 ? "off" : "on")); break;                                       // On/Off switch 0 to 63 = Off, 64 to 127 = On (GM2)
        case 0x42: ::snprintf(Line.data(), Line.size(), "Sostenuto %s", (Value < 64 ? "off" : "on")); break;                                        // On/Off switch. Like the Sustain controller (CC 64), However it only holds notes that were “On” when the pedal was pressed. People use it to “hold” chords” and play melodies over the held chord. 0 to 63 = Off, 64 to 127 = On (GM2)
        case 0x43: ::snprintf(Line.data(), Line.size(), "Soft Pedal %s", (Value < 64 ? "off" : "on")); break;                                       // Lowers the volume of notes played. 0 to 63 = Off, 64 to 127 = On (GM2)
        case 0x44: ::snprintf(Line.data(), Line.size(), "Legato Footswitch: %s", (Value < 64 ? "Normal" : "Legato")); break;                        // Turns Legato effect between 2 subsequent notes On or Off. 0 to 63 = Off, 64 to 127 = On
        case 0x45: ::snprintf(Line.data(), Line.size(), "Hold 2 %s", (Value < 64 ? "off" : "on")); break;                                           // Another way to “hold notes” (see MIDI CC 64 and MIDI CC 66). However notes fade out according to their release parameter rather than when the pedal is released.

        // XG / GM2
        case 0x46: ::snprintf(Line.data(), Line.size(), "Sound Controller 1 (Sound Variation) %d", Value); break;                                  // Usually controls the way a sound is produced. Default = Sound Variation.
        case 0x47: ::snprintf(Line.data(), Line.size(), "Sound Controller 2 (Timbre /Harmonic Intensity) %d", Value); break;                       // Allows shaping the Voltage Controlled Filter (VCF). Default = Resonance - also(Timbre or Harmonics) (GM2)
        case 0x48: ::snprintf(Line.data(), Line.size(), "Sound Controller 3 (Release Time) %s", (Value < 64 ? "shorter" : "longer")); break;       // Controls release time of the Voltage controlled Amplifier (VCA). Default = Release Time. (GM2)
        case 0x49: ::snprintf(Line.data(), Line.size(), "Sound Controller 4 (Attack Time) %s", (Value < 64 ? "shorter" : "longer")); break;        // Controls the “Attack’ of a sound. The attack is the amount of time it takes forthe sound to reach maximum amplitude. (GM2)
        case 0x4A: ::snprintf(Line.data(), Line.size(), "Sound Controller 5 (Brightness) %s", (Value < 64 ? "lower" : "higher")); break;           // Controls VCF's cutoff frequency of the filter. (GM2)
        case 0x4B: ::snprintf(Line.data(), Line.size(), "Sound Controller 6 (Decay Time) %s", (Value < 64 ? "shorter" : "longer")); break;         // Generic – Some manufacturers may use to further shave their sounds. (GM2)
        case 0x4C: ::snprintf(Line.data(), Line.size(), "Sound Controller 7 (Vibrato Rate) %s", (Value < 64 ? "slower" : "faster")); break;        // Generic – Some manufacturers may use to further shave their sounds. (GM2)
        case 0x4D: ::snprintf(Line.data(), Line.size(), "Sound Controller 8 (Vibrato Depth) %s", (Value < 64 ? "reduced" : "increased")); break;   // Generic – Some manufacturers may use to further shave their sounds. (GM2)
        case 0x4E: ::snprintf(Line.data(), Line.size(), "Sound Controller 9 (Vibrato Delay) %s", (Value < 64 ? "shorter" : "longer")); break;      // Generic – Some manufacturers may use to further shave their sounds. (GM2)
        case 0x4F: ::snprintf(Line.data(), Line.size(), "Sound Controller 10 (Metronome Rate) %d", Value); break;                                  // Generic – Some manufacturers may use to further shave their sounds.

        case 0x50: ::snprintf(Line.data(), Line.size(), "Generic On/Off switch: %s", (Value < 64 ? "off" : "one")); break;
        case 0x51: ::snprintf(Line.data(), Line.size(), "Generic On/Off switch: %s", (Value < 64 ? "off" : "one")); break;
        case 0x52: ::snprintf(Line.data(), Line.size(), "Generic On/Off switch: %s", (Value < 64 ? "off" : "one")); break;
        case 0x53: ::snprintf(Line.data(), Line.size(), "Generic On/Off switch: %s", (Value < 64 ? "off" : "one")); break;
        case 0x54: ::snprintf(Line.data(), Line.size(), "Portamento Amount %d", Value); break;

        case 0x58: ::snprintf(Line.data(), Line.size(), "High Resolution Velocity Prefix %d", Value); break;                                        // Extends range of values, thereby creating a greater degree of precision.

        case 0x5B: ::snprintf(Line.data(), Line.size(), "Effect 1 Depth (Reverb) Send Level %d", Value); break;                                     // (GM2)
        case 0x5C: ::snprintf(Line.data(), Line.size(), "Effect 2 Depth (Tremolo) Send Level %d", Value); break;
        case 0x5D: ::snprintf(Line.data(), Line.size(), "Effect 3 Depth (Chorus) Send Level %d", Value); break;                                     // (GM2)
        case 0x5E: ::snprintf(Line.data(), Line.size(), "Effect 4 Depth (Detune) Send Level %d", Value); break;
        case 0x5F: ::snprintf(Line.data(), Line.size(), "Effect 5 Depth (Phaser) Send Level %d", Value); break;

        case 0x60: ::snprintf(Line.data(), Line.size(), "Data Increment"); break;
        case 0x61: ::snprintf(Line.data(), Line.size(), "Data Decrement"); break;

        case 0x62: ::snprintf(Line.data(), Line.size(), "Select NRPN parameter (LSB) %d", Value); break;
        case 0x63: ::snprintf(Line.data(), Line.size(), "Select NRPN parameter (MSB) %d", Value); break;

        case 0x64: ::snprintf(Line.data(), Line.size(), "Select RPN parameter (LSB) %d", Value); break;                                             // (GM2)
        case 0x65: ::snprintf(Line.data(), Line.size(), "Select RPN parameter (MSB) %d", Value); break;                                             // (GM2)

        case 0x6E: ::snprintf(Line.data(), Line.size(), "LeapFrog Start of Loop marker (%d %d)", (int) d1, Value); break;
        case 0x6F: ::snprintf(Line.data(), Line.size(), "LeapFrog End of Loop marker (%d %d)", (int) d1, Value); break;

        // Channel Mode Messages
        case 0x78: ::snprintf(Line.data(), Line.size(), "All Sound Off"); break;           // Mutes all sounding notes. It does so regardless of release time or sustain. (See MIDI CC 123)
        case 0x79: ::snprintf(Line.data(), Line.size(), "Reset All Controllers"); break;   // It will reset all controllers to their default.
        case 0x7A: ::snprintf(Line.data(), Line.size(), "Local Control"); break;           // Turns internal connection of a MIDI keyboard/workstation, etc. On or Off. If you use a computer, you will most likely want local control off to avoid notes being played twice. Once locally and twice whent the note is sent back from the computer to your keyboard.
        case 0x7B: ::snprintf(Line.data(), Line.size(), "All Notes Off"); break;           // Mutes all sounding notes. Release time will still be maintained, and notes held by sustain will not turn off until sustain pedal is depressed.
        case 0x7C: ::snprintf(Line.data(), Line.size(), "Omni Off"); break;
        case 0x7D: ::snprintf(Line.data(), Line.size(), "Omni On"); break;
        case 0x7E: ::snprintf(Line.data(), Line.size(), "Mono On (Poly Off)"); break;      // Sets device mode to Monophonic. [Channel Mode Message] Mono Mode On (+ poly off, + all notes off). This equals the number of channels, or zero if the number of channels equals the number of voices in the receiver.
        case 0x7F: ::snprintf(Line.data(), Line.size(), "Poly On (Mono Off)"); break;      // Sets device mode to Polyphonic. [Channel Mode Message] Poly Mode On (+ mono off, +all notes off).

        default:
            if (InRange((int) d1, 20, 31))
                ::snprintf(Line.data(), Line.size(), "Undefined");
            else
            if (InRange((int) d1, 85, 87))
                ::snprintf(Line.data(), Line.size(), "Undefined");
            else
            if (InRange((int) d1, 89, 90))
                ::snprintf(Line.data(), Line.size(), "Undefined");
            else
                ::snprintf(Line.data(), Line.size(), "Unknown CC %02Xh%3d %02X", (int) d1, (int) d1, Value); break;
    }
/*
    if (Event[1] == 98)     // Non-Registered Parameter LSB
        CCLSB = Event[2];
    else
    if (Event[1] == 99)     // Non-Registered Parameter MSB
        CCMSB = Event[2];
    else
    if (Event[1] == 100)    // Registered Parameter LSB
        CCLSB = Event[2];
    else
    if (Event[1] == 101)    // Registered Parameter MSB
        CCMSB = Event[2];
    else
    if (Event[1] == 38)     // LSB for CC 0 - 31.
        DELSB = Event[2];

    if ((CCLSB != 255) && (CCMSB != 255))
    {
        int RPN = (CCMSB << 7) | CCLSB; // Registered Parameter Number

        if ((Event[1] == 6) || (Event[1] == 96) || (Event[1] == 97))
        {
            int Value = (Event[2] << 7) | DELSB;

            if (Event[1] == 6)      // Data Entry
                ::printf("Set RPN 0x%04X to %3d\n", RPN, Value);
            else
            if (Event[1] == 96)
                ::printf("Increment RPN 0x%04X by %3d\n", RPN, Value);
            else
            if (Event[1] == 97)
                ::printf("Decrement RPN 0x%04X by %3d\n", RPN, Value);

            CCMSB = CCLSB = 255;
            DELSB = 0;
        }
    }
*/
    return Line;
}
