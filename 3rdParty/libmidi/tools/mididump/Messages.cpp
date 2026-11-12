
/** $VER: Messages.cpp (2026.05.10) P. Stuer **/

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

const std::unordered_map<uint8_t, const char *> GM1DrumSets =
{
    {   0, "Standard Drum Set" },
    {   8, "Room Drum Set" },
    {  16, "Power Drum Set" },
    {  24, "Electronic Drum Set" },
    {  25, "Analog Drum Set" },
    {  32, "Jazz Drum Set" },
    {  40, "Brush Drum Set" },
    {  48, "Orchestra Drum Set" },
    {  56, "SFX Drum Set" },
};

// Roland SC-8850 Owner's Manual, p. 187, "Drum Set List"
const std::unordered_map<uint8_t, const char *> GSDrumSets =
{
    {   0, "Standard 1 Drum Set" },
    {   1, "Standard 2 Drum Set" },     // SC-88 or later
    {   2, "Standard L/R Drum Set" },   // SC-88Pro or later

    {   8, "Room Drum Set" },
    {   9, "Hip Hop Drum Set" },        // SC-88Pro or later
    {  10, "Jungle Drum Set" },         // SC-88Pro or later
    {  11, "Techno Drum Set" },         // SC-88Pro or later
    {  12, "Room L/R Drum Set" },       // SC-8850 or later
    {  13, "House Drum Set" },          // SC-8850 or later

    {  16, "Power Drum Set" },

    {  24, "Electronic Drum Set" },
    {  25, "TR-808 Drum Set" },
    {  26, "Dance Drum Set" },          // SC-88 or later
    {  27, "CR-78 Drum Set" },          // SC-88Pro or later
    {  28, "TR-606 Drum Set" },         // SC-88Pro or later
    {  29, "TR-707 Drum Set" },         // SC-88Pro or later
    {  30, "TR-909 Drum Set" },         // SC-88Pro or later

    {  32, "Jazz Drum Set" },
    {  33, "Jazz L/R Drum Set" },       // SC-8850 or later

    {  40, "Brush Drum Set" },
    {  41, "Brush 2 Drum Set" },        // SC-8850 or later
    {  42, "Brush 2 L/R Drum Set" },    // SC-8850 or later

    {  48, "Orchestra Drum Set" },
    {  49, "Ethnic Drum Set" },         // SC-88 or later
    {  50, "Kick & Snare Drum Set" },   // SC-88 or later
    {  51, "Kick & Snare 2 Drum Set" }, // SC-8850 or later
    {  52, "Asia Drum Set" },           // SC-88Pro or later
    {  53, "Cymbal & Claps Drum Set" }, // SC-88Pro or later
    {  54, "Gamelan 1 Drum Set" },      // SC-8850 or later
    {  55, "Gamelan 2 Drum Set" },      // SC-8850 or later

    {  56, "SFX Drum Set" },
    {  57, "Rhythm FX Drum Set" },      // SC-88 or later
    {  58, "Rhythm FX 2 Drum Set" },    // SC-88Pro or later
    {  59, "Rhythm FX 3 Drum Set" },    // SC-8850 or later
    {  60, "SFX 2 Drum Set" },          // SC-8850 or later
    {  61, "Voice Drum Set" },          // SC-8850 or later
    {  62, "Cym & Claps 2 Drum Set" },  // SC-8850 or later

    { 127, "CM-64/CM-32L Drum Set" },   // SC-55 only
};

// General MIDI Level 1 Standard Drums
const std::unordered_map<uint8_t, const char *> GMStandardDrums =
{
    { 35, "Acoustic Bass Drum" },
    { 36, "Bass Drum 1" },
    { 37, "Side Stick" },
    { 38, "Acoustic Snare" },
    { 39, "Hand Clap" },
    { 40, "Electric Snare" },
    { 41, "Low Floor Tom" },
    { 42, "Closed Hi-Hat" },
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

// GS Drum Set 1 "Standard 1", Roland SC-8850 Owner's Manual, p. 188
const std::unordered_map<uint8_t, const char *> GSStandard1Drums =
{
    { 22, "MC-500 Beep 1" },        // SC8850 or later
    { 23, "MC-500 Beep 2" },        // SC8850 or later
    { 24, "Concert SD" },           // SC8850 or later
    { 25, "Snare Roll" },           // SC8850 or later
    { 26, "Finger Snap 2" },        // SC8850 or later
    { 27, "High Q" },
    { 28, "Slap" },
    { 29, "Scratch Push" },
    { 30, "Scratch Pull" },
    { 31, "Sticks" },
    { 32, "Square Click" },
    { 33, "Metronome Click" },
    { 34, "Metronome Bell" },

    { 35, "Standard 1 Kick 2" },
    { 36, "Standard 1 Kick 1" },
    { 37, "Side Stick" },
    { 38, "Standard 1 Snare 1" },
    { 39, "TR-909 Hand Clap" },
    { 40, "Standard 1 Snare 2" },
    { 41, "Low Tom 2" },
    { 42, "Closed Hi-Hat 1" },
    { 43, "Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Mid Tom 2" },
    { 46, "Open Hi-Hat 1" },
    { 47, "Mid Tom 1" },
    { 48, "High Tom 2" },
    { 49, "Crash Cymbal 1" },
    { 50, "High Tom 1" },
    { 51, "Ride Cymbal 1" },
    { 52, "Chinese Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Tambourine" },
    { 55, "Splash Cymbal" },
    { 56, "Cowbell" },
    { 57, "Crash Cymbal 2" },
    { 58, "Vibra-slap" },
    { 59, "Ride Cymbal 2" },
    { 60, "High Bongo" },
    { 61, "Low Bongo" },
    { 62, "Mute High Conga" },
    { 63, "Open High Conga" },
    { 64, "Low Conga" },
    { 65, "High Timbale" },
    { 66, "Low Timbale" },
    { 67, "High Agogo" },
    { 68, "Low Agogo" },
    { 69, "Cabasa" },
    { 70, "Maracas" },
    { 71, "Short High Whistle" },
    { 72, "Long Low Whistle" },
    { 73, "Short Guiro" },
    { 74, "Long Guiro" },
    { 75, "Claves" },
    { 76, "High Wood Block" },
    { 77, "Low Wood Block" },
    { 78, "Mute Cuica" },
    { 79, "Open Cuica" },
    { 80, "Mute Triangle" },
    { 81, "Open Triangle" },

    { 82, "Shaker" },
    { 83, "Jingle Bell" },
    { 84, "Bell Tree" },
    { 85, "Castanets" },
    { 86, "Mute Surdo" },
    { 87, "Open Surdo" },
    { 88, "Applause 2" },           // SC8850 or later

    { 95, "Room Snare 1" },         // SC8850 or later
    { 96, "Room Snare 2" },         // SC8850 or later
};

// GS Drum Set 2 "Standard 2", Roland SC-8850 Owner's Manual, p. 188
const std::unordered_map<uint8_t, const char *> GSStandard2Drums =
{
    { 26, "Finger Snap" },

    { 35, "Standard 2 Kick 2" },
    { 36, "Standard 2 Kick 1" },

    { 38, "Standard 2 Snare 1" },
    { 39, "Hand Clap" },
    { 40, "Standard 2 Snare 2" },

    { 42, "Closed Hi-Hat" },

    { 44, "Pedal Hi-Hat" },

    { 84, "Bar Chimes" },
};

// GS Drum Set 3 "Standard L/R", Roland SC-8850 Owner's Manual, p. 188
const std::unordered_map<uint8_t, const char *> GSStandardLRDrums =
{
    { 35, "Standard Kick 2" },
    { 36, "Standard Kick 1" },

    { 38, "Standard Snare 1" },

    { 40, "Standard Snare 2" },
    { 41, "Low Tom 2" },
    { 42, "Closed Hi-Hat" },
    { 43, "Low Tom 1" },

    { 45, "Mid Tom 2" },
    { 46, "Open Hi-Hat" },
    { 47, "Mid Tom 1" },
    { 48, "High Tom 2" },
    { 49, "Crash Cymbal" },
    { 50, "High Tom 1" },
    { 51, "Ride Cymbal 1" },

    { 53, "Ride Bell 1" },

    { 95, "Standard 1 Snare 1" },
    { 96, "Standard 1 Snare 2" },
};

// GS Drum Set 9 "Room", Roland SC-8850 Owner's Manual, p. 188
const std::unordered_map<uint8_t, const char *> GSRoomDrums =
{
    { 26, "Finger Snap" },

    { 35, "Room Kick 2" },
    { 36, "Room Kick 1" },

    { 38, "Room Snare 1" },
    { 39, "Hand Clap" },
    { 40, "Room Snare 2" },
    { 41, "Room Low Tom 2" },
    { 42, "Room Closed Hi-Hat 3" },
    { 43, "Room Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Room Mid Tom 2" },
    { 46, "Room Open Hi-Hat 3" },
    { 47, "Room Mid Tom 1" },
    { 48, "Room High Tom 2" },
    { 49, "Room Crash Cymbal" },
    { 50, "Room High Tom 1" },
    { 51, "Room Ride Cymbal" },

    { 53, "Room Ride Bell" },

    { 95, "Standard 1 Snare 1" },
    { 96, "standard 1 Snare 2" },
};

// GS Drum Set 10 "Hip Hop", Roland SC-8850 Owner's Manual, p. 188
const std::unordered_map<uint8_t, const char *> GSHipHopDrums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "Hip-Hop Kick 2" },
    { 36, "Hip-Hop Kick 1" },
    { 37, "TR-808 Rim Shot" },
    { 38, "LoFi Snare 1" },
    { 39, "TR-707 Claps" },
    { 40, "LoFi Snare 2" },
    { 41, "Jazz Low Tom 2" },
    { 42, "Room Closed Hi-Hat" },
    { 43, "Jazz Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Jazz Mid Tom 2" },
    { 46, "Room Open Hi-Hat" },
    { 47, "Jazz Mid Tom 1" },
    { 48, "Jazz High Tom 2" },
    { 49, "TR-909 Crash Cymbal" },
    { 50, "Jazz High Tom 1" },

    { 52, "Reverse Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Shake Tambourine" },

    { 56, "TR-808 Cowbell" },

    { 70, "TR-808 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "TR-808 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "Shaker 2" },

    { 88, "Small Club 1" },
    { 89, "Hip-Hop Snare 2" },
    { 90, "LoFi Snare Rim" },
    { 91, "Hip-Hop Claps" },
    { 92, "Standard 1 Snare 1" },
    { 93, "Standard 1 Snare 2" },
    { 94, "Room Snare 1" },
    { 95, "Room Snare 2" },
    { 96, "Dance Snare" },
};

// GS Drum Set 11 "Jungle", Roland SC-8850 Owner's Manual, p. 189
const std::unordered_map<uint8_t, const char *> GSJungleDrums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "Jungle Kick 2" },
    { 36, "Jungle Kick 1" },
    { 37, "Jungle Snare Rim" },
    { 38, "Hip-Hop Snare 1" },
    { 39, "R&B Claps" },
    { 40, "Jungle Snare" },
    { 41, "TR-909 Low Tom 2" },
    { 42, "TR-606 Closed Hi-Hat" },
    { 43, "TR-909 Low Tom 1" },
    { 44, "Jungle Hi-Hat" },
    { 45, "TR-909 Mid Tom 2" },
    { 46, "TR-606 Open Hi-Hat" },
    { 47, "TR-909 Mid Tom 1" },
    { 48, "TR-909 High Tom 2" },
    { 49, "Jungle Crash" },
    { 50, "TR-909 High Tom 1" },
    { 51, "Ride Cymbal 1" },
    { 52, "Reverse Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Shake Tambourine" },

    { 56, "TR-808 Cowbell" },

    { 70, "TR-808 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "TR-808 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "Jungle Shaker" },

    { 88, "Small Club 1" },
    { 89, "Jungle Kick Roll" },
    { 90, "Jungle Snare Roll" },
    { 91, "TR-606 Snare 2" },
    { 92, "Dance Snare" },
    { 93, "Techo Snare" },
    { 94, "House Snare" },
    { 95, "Rock Snare Dry" },
    { 96, "LoFi Snare 1" },
};

// GS Drum Set 12 "Techno", Roland SC-8850 Owner's Manual, p. 189
const std::unordered_map<uint8_t, const char *> GSTechnoDrums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "TR-808 Kick" },
    { 36, "TR-909 Kick 1" },
    { 37, "TR-909 Snare Rim" },
    { 38, "TR-606 Snare 2" },
    { 39, "TR-909 Claps" },
    { 40, "Techno Snare" },
    { 41, "TR-606 Low Tom 2" },
    { 42, "TR-707 Closed Hi-Hat" },
    { 43, "TR-606 Low Tom 1" },
    { 44, "CR-78 Closed Hi-Hat" },
    { 45, "TR-606 Mid Tom 2" },
    { 46, "TR-909 Open Hi-Hat" },
    { 47, "TR-606 Mid Tom 1" },
    { 48, "TR-606 High Tom 2" },
    { 49, "TR-909 Crash Cymbal" },
    { 50, "TR-606 High Tom 1" },
    { 51, "Ride Cymbal 1" },
    { 52, "Reverse Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Shake Tambourine" },

    { 56, "TR-808 Cowbell" },
    { 57, "TR-909 Crash Cymbal" },

    { 60, "CR-78 High Bongo" },
    { 61, "CR-78 Low Bongo" },
    { 62, "TR-808 High Conga" },
    { 63, "TR-808 Mute Conga" },
    { 64, "TR-808 Low Conga" },

    { 70, "TR-808 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "TR-808 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },

    { 89, "Dance Snare" },
    { 90, "House Snare" },
    { 91, "Rock Snare Dry" },
    { 92, "Jungle Snare" },
    { 93, "LoFi Snare 1" },
    { 94, "LoFi Snare 2" },
    { 95, "Hip-Hop Snare 1" },
    { 96, "Hip-Hop Snare 2" },
};

// GS Drum Set 13 "Room L/R", Roland SC-8850 Owner's Manual, p. 189
const std::unordered_map<uint8_t, const char *> GSRoomLRDrums =
{
    { 26, "Finger Snap" },

    { 35, "Room Kick 2" },
    { 36, "Room Kick 1" },

    { 38, "Room Snare 1" },
    { 39, "Hard Clap" },
    { 40, "Room Snare 2" },
    { 41, "Room Low Tom 2" },
    { 42, "Room Closed Hi-Hat" },
    { 43, "Room Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Room Mid Tom 2" },
    { 46, "Room Open Hi-Hat" },
    { 47, "Room Mid Tom 1" },
    { 48, "Room High Tom 2" },
    { 49, "Room Crash Cymbal" },
    { 50, "Room High Tom 1" },
    { 51, "Room Ride Cymbal" },

    { 53, "Room Ride Bell" },

    { 55, "Splash Cymbal" },

    { 95, "Room Kick 2" },
    { 96, "Room Kick 1" },
};

// GS Drum Set 14 "House", Roland SC-8850 Owner's Manual, p. 189
const std::unordered_map<uint8_t, const char *> GSHouseDrums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "TR-909 Kick 2" },
    { 36, "TR-909 Kick 1" },
    { 37, "House Snare Rim" },
    { 38, "House Snare 1" },
    { 39, "TR-909 Clap" },
    { 40, "House Snare 2" },
    { 41, "TR-9090 Low Tom 2" },
    { 42, "Room Closed Hi-Hat" },
    { 43, "TR-909 Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "TR-909 Mid Tom 2" },
    { 46, "Room Open Hi-Hat" },
    { 47, "TR-909 Mid Tom 1" },
    { 48, "TR-909 High Tom 2" },
    { 49, "TR-909 Crash Cymbal" },
    { 50, "TR-909 High Tom 1" },
    { 51, "TR-909 Ride Cymbal" },
    { 52, "Reverse Cymbal" },
    { 53, "Ride Bell" },
    { 54, "Shake Tambourine" },

    { 56, "TR-808 Cowbell" },
    { 57, "TR-909 Crash Cymbal" },

    { 60, "CR-78 High Bongo" },
    { 61, "CR-78 Low Bongo" },
    { 62, "TR-808 High Conga" },
    { 63, "TR-808 Mute Conga" },
    { 64, "TR-808 Low Conga" },

    { 70, "TR-808 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "TR-808 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
    { 89, "TR-606 Snare 2" },
    { 90, "Dance Snare" },
    { 91, "Techno Snare" },
    { 92, "Rock Snare Dry" },
    { 93, "Hip-Hop Snare 1" },
    { 94, "LoFi Snare 1" },
    { 95, "LoFi Snare 2" },
    { 96, "Jungle Snare" },
};

// GS Drum Set 17 "Power", Roland SC-8850 Owner's Manual, p. 189
const std::unordered_map<uint8_t, const char *> GSPowerDrums =
{
    { 35, "Power Kick 2" },
    { 36, "Power Kick 1" },

    { 38, "Power Snare 1" },
    { 39, "Hand Clap" },
    { 40, "Power Snare 2" },
    { 41, "Power Low Tom 2" },

    { 43, "Power Low Tom 1" },

    { 45, "Power Mid Tom 2" },

    { 47, "Power Mid Tom 1" },
    { 48, "Power High Tom 2" },

    { 50, "Power High Tom 1" },
};

// GS Drum Set 18 "Electronic", Roland SC-8850 Owner's Manual, p. 190
const std::unordered_map<uint8_t, const char *> GSElectronicDrums =
{
    { 26, "Finger Snap 2" },

    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "Electic Kick 2" },
    { 36, "Electric Kick 1" },

    { 38, "Electric Snare 1" },
    { 39, "Hand Clap" },
    { 40, "Electric Snare 2" },
    { 41, "Electic Low Tom 2" },
    { 42, "Closed Hi-Hat 2" },
    { 43, "Electic Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Electric Mid Tom 2" },
    { 46, "Open Hi-Hat 2" },
    { 47, "Electric Mid Tom 1" },
    { 48, "Electric High Tom 2" },

    { 50, "Electric High Tom 1" },

    { 88, "Small Club 1" },
};

// GS Drum Set 26 "TR-808", Roland SC-8850 Owner's Manual, p. 190
const std::unordered_map<uint8_t, const char *> GSTR808Drums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "TR-808 Kick 2" },
    { 36, "TR-909 Kick 1" },
    { 37, "TR-808 Rim Shot" },
    { 38, "TR-808 Snare 1" },
    { 39, "Hand Clap" },
    { 40, "TR-808 Snare 2" },
    { 41, "TR-808 Low Tom 2" },
    { 42, "TR-808 Closed Hi-Hat 2" },
    { 43, "TR-808 Low Tom 1" },
    { 44, "TR-808 Closed Hi-Hat" },
    { 45, "TR-808 Mid Tom 2" },
    { 46, "TR-808 Open Hi-Hat" },
    { 47, "TR-808 Mid Tom 1" },
    { 48, "TR-808 High Tom 2" },
    { 49, "TR-808 Crash Cymbal" },
    { 50, "TR-808 High Tom 1" },
    { 51, "TR-606 Ride Cymbal" },

    { 54, "CR-78 Tambourine" },

    { 56, "TR-808 Cowbell" },
    { 57, "TR-909 Crash Cymbal" },

    { 59, "Ride Cymbal 2" },
    { 60, "CR-78 High Bongo" },
    { 61, "CR-78 Low Bongo" },
    { 62, "TR-808 High Conga" },
    { 63, "TR-808 Mute Conga" },
    { 64, "TR-808 Low Conga" },

    { 70, "TR-808 Maracas" },
    { 75, "TR-808 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
};

// GS Drum Set 27 "Dance", Roland SC-8850 Owner's Manual, p. 190
const std::unordered_map<uint8_t, const char *> GSDanceDrums =
{
    { 26, "Finger Snap 2" },

    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "Fat Kick" },
    { 36, "Dance Kick" },
    { 37, "Dance Rim Shot" },
    { 38, "Dance Snare" },
    { 39, "Comp Claps 2" },
    { 40, "Rock SD Dry" },
    { 41, "Electric Low Tom 2" },
    { 42, "CR-78 Closed Hi-Hat" },
    { 43, "Electric Low Tom 1" },
    { 44, "TR-808 Closed Hi-Hat 2" },
    { 45, "Electric Mid Tom 2" },
    { 46, "CR-78 Open Hi-Hat" },
    { 47, "Electric Mid Tom 1" },
    { 48, "Electric High Tom 2" },
    { 49, "TR-808 Crash Cymbal" },
    { 50, "Electric High Tom 1" },
    { 51, "TR-606 Ride Cymbal" },
    { 52, "Reverse Cymbal" },
    { 51, "Ride Bell" },
    { 54, "Shake Tambourine" },

    { 56, "TR-808 Cowbell" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
    { 89, "TR-606 Snare 2" },
    { 90, "Techno Snare" },
    { 91, "House Snare" },
    { 92, "Jungle Snare" },
    { 93, "LoFi Snare 1" },
    { 94, "LoFi Snare 2" },
    { 95, "Hip-Hop Snare 1" },
    { 95, "Hip-Hop Snare 2" },
};

// GS Drum Set 28 "CR-78", Roland SC-8850 Owner's Manual, p. 190
const std::unordered_map<uint8_t, const char *> GSCR78Drums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "CR-78 Kick 2" },
    { 36, "CR-78 Kick 1" },
    { 37, "CR-78 Rim Shot" },
    { 38, "CR-78 Snare 1" },
    { 39, "TR-707 Hand Clap" },
    { 40, "CR-78 Snare 2" },
    { 41, "CR-78 Low Tom 2" },
    { 42, "CR-78 Closed Hi-Hat 2" },
    { 43, "CR-78 Low Tom 1" },
    { 44, "TR-606 Closed Hi-Hat" },
    { 45, "CR-78 Mid Tom 2" },
    { 46, "CR-78 Open Hi-Hat" },
    { 47, "CR-78 Mid Tom 1" },
    { 48, "CR-78 High Tom 2" },
    { 49, "TR-808 Crash Cymbal" },
    { 50, "CR-78 High Tom 1" },
    { 51, "CR-78 Ride Cymbal" },

    { 54, "CR-78 Tambourine" },

    { 56, "CR-78 Cowbell" },
    { 57, "TR-909 Crash Cymbal" },

    { 59, "Ride Cymbal Edge" },
    { 60, "CR-78 High Bongo" },
    { 61, "CR-78 Low Bongo" },
    { 62, "TR-808 High Conga" },
    { 63, "TR-808 Mute Conga" },
    { 64, "TR-808 Low Conga" },

    { 70, "CR-78 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "CR-78 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "CR-78 Metalic Beat 1" },
    { 81, "CR-78 Metalic Beat 2" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
};

// GS Drum Set 29 "TR-606", Roland SC-8850 Owner's Manual, p. 190
const std::unordered_map<uint8_t, const char *> GSTR606Drums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "CR-78 Kick 2" },
    { 36, "TR-606 Kick 1" },
    { 37, "CR-78 Rim Shot" },
    { 38, "TR-606 Snare 1" },
    { 39, "TR-707 Hand Clap" },
    { 40, "TR-606 Snare 2" },
    { 41, "TR-606 Low Tom 2" },
    { 42, "TR-606 Closed Hi-Hat 2" },
    { 43, "TR-606 Low Tom 1" },
    { 44, "TR-606 Closed Hi-Hat" },
    { 45, "TR-606 Mid Tom 2" },
    { 46, "TR-606 Open Hi-Hat" },
    { 47, "TR-606 Mid Tom 1" },
    { 48, "TR-606 High Tom 2" },
    { 49, "TR-606 Crash Cymbal" },
    { 50, "TR-606 High Tom 1" },
    { 51, "TR-606 Ride Cymbal" },

    { 54, "CR-78 Tambourine" },

    { 56, "CR-78 Cowbell" },
    { 57, "TR-909 Crash Cymbal" },

    { 59, "Ride Cymbal Edge" },
    { 60, "CR-78 High Bongo" },
    { 61, "CR-78 Low Bongo" },
    { 62, "TR-808 High Conga" },
    { 63, "TR-808 Mute Conga" },
    { 64, "TR-808 Low Conga" },

    { 70, "CR-78 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "CR-78 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "CR-78 Metalic Beat 1" },
    { 81, "CR-78 Metalic Beat 2" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
};

// GS Drum Set 30 "TR-707", Roland SC-8850 Owner's Manual, p. 191
const std::unordered_map<uint8_t, const char *> GSTR707Drums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "TR-707 Kick 2" },
    { 36, "TR-707 Kick 1" },
    { 37, "TR-707 Rim Shot" },
    { 38, "TR-707 Snare 1" },
    { 39, "TR-707 Hand Clap" },
    { 40, "TR-707 Snare 2" },
    { 41, "TR-707 Low Tom 2" },
    { 42, "TR-707 Closed Hi-Hat" },
    { 43, "TR-707 Low Tom 1" },
    { 44, "TR-707 Closed Hi-Hat" },
    { 45, "TR-707 Mid Tom 2" },
    { 46, "TR-707 Open Hi-Hat" },
    { 47, "TR-707 Mid Tom 1" },
    { 48, "TR-707 High Tom 2" },
    { 49, "TR-707 Crash Cymbal" },
    { 50, "TR-707 High Tom 1" },
    { 51, "TR-707 Ride Cymbal" },

    { 54, "Tambourine 2" },

    { 56, "TR-808 Cowbell" },

    { 59, "Ride Cymbal Edge" },

    { 70, "TR-808 Maracas" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },

    { 88, "Small Club 1" },
};

// GS Drum Set 31 "TR-909", Roland SC-8850 Owner's Manual, p. 191
const std::unordered_map<uint8_t, const char *> GSTR909Drums =
{
    { 29, "Scratch Push 2" },
    { 30, "Scratch Pull 2" },

    { 35, "TR-909 Kick 2" },
    { 36, "TR-909 Kick 1" },
    { 37, "TR-909 Rim" },
    { 38, "TR-909 Snare 1" },

    { 40, "TR-909 Snare 2" },
    { 41, "TR-909 Low Tom 2" },
    { 42, "TR-707 Closed Hi-Hat" },
    { 43, "TR-909 Low Tom 1" },
    { 44, "TR-707 Closed Hi-Hat" },
    { 45, "TR-909 Mid Tom 2" },
    { 46, "TR-909 Open Hi-Hat" },
    { 47, "TR-909 Mid Tom 1" },
    { 48, "TR-909 High Tom 2" },
    { 49, "TR-909 Crash Cymbal" },
    { 50, "TR-909 High Tom 1" },
    { 51, "TR-909 Ride Cymbal" },

    { 54, "Tambourine 2" },

    { 56, "TR-808 Cowbell" },

    { 59, "Ride Cymbal Edge" },

    { 70, "TR-808 Maracas" },

    { 74, "CR-78 Guiro" },
    { 75, "CR-78 Claves" },

    { 78, "High Hoo" },
    { 79, "Low Hoo" },
    { 80, "Electric Mute Triangle" },
    { 81, "Electric Open Triangle" },
    { 82, "TR-626 Shaker" },
};

// GS Drum Set 33 "Jazz", Roland SC-8850 Owner's Manual, p. 191
const std::unordered_map<uint8_t, const char *> GSJazzDrums =
{
    { 26, "Finger Snap 2" },

    { 35, "Jazz Kick 2" },
    { 36, "Jazz Kick 1" },

    { 38, "Jazz Snare 1" },
    { 39, "Hand Clap 2" },
    { 40, "Jazz Snare 1" },
    { 41, "Jazz Low Tom 2" },
    { 42, "Closed Hi-Hat 2" },
    { 43, "Jazz Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Jazz Mid Tom 2" },
    { 46, "Open Hi-Hat 2" },
    { 47, "Jazz Mid Tom 1" },
    { 48, "Jazz High Tom 2" },
    { 49, "Jazz Crash Cymbal" },
    { 50, "Jazz High Tom 1" },
    { 51, "Jazz Ride Cymbal" },

    { 53, "Jazz Ride Bell" },

    { 59, "Ride Cymbal Edge" },

    { 88, "Applause" },
};

// GS Drum Set 34 "Jazz L/R", Roland SC-8850 Owner's Manual, p. 191
const std::unordered_map<uint8_t, const char *> GSJazzLRDrums =
{
    { 26, "Finger Snap 2" },

    { 35, "Jazz Kick 2" },
    { 36, "Jazz Kick 1" },

    { 38, "Jazz Snare 1" },
    { 39, "Hand Clap 2" },
    { 40, "Jazz Snare 1" },
    { 41, "Jazz Low Tom 2" },
    { 42, "Jazz Closed Hi-Hat" },
    { 43, "Jazz Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Jazz Mid Tom 2" },
    { 46, "Jazz Open Hi-Hat" },
    { 47, "Jazz Mid Tom 1" },
    { 48, "Jazz High Tom 2" },
    { 49, "Jazz Crash Cymbal" },
    { 50, "Jazz High Tom 1" },
    { 51, "Jazz Ride Cymbal" },

    { 53, "Jazz Ride Bell" },

    { 59, "Ride Cymbal Edge" },

    { 88, "Applause" },

    { 95, "Jazz Kick 2" },
    { 96, "Jazz Kick 1" },
};

// GS Drum Set 41 "Brush", Roland SC-8850 Owner's Manual, p. 191
const std::unordered_map<uint8_t, const char *> GSBrushDrums =
{
    { 26, "Finger Snap 2" },

    { 35, "Jazz Kick 2" },
    { 36, "Jazz Kick 1" },

    { 38, "Brush Tap 1" },
    { 39, "Brush Slap 1" },
    { 40, "Brush Swirl 1" },
    { 41, "Brush Low Tom 2" },
    { 42, "Brush Closed Hi-Hat" },
    { 43, "Brush Low Tom 1" },
    { 44, "Pedal Hi-Hat" },
    { 45, "Brush Mid Tom 2" },
    { 46, "Brush Open Hi-Hat" },
    { 47, "Brush Mid Tom 1" },
    { 48, "Brush High Tom 2" },
    { 49, "Brush Crash Cymbal" },
    { 50, "Brush High Tom 1" },
    { 51, "Ride Cymbal Inner" },

    { 53, "Brush Ride Bell" },

    { 59, "Ride Cymbal Edge" },

    { 88, "Applause" },
};

// GS Drum Set 42 "Brush 2", Roland SC-8850 Owner's Manual, p. 192
const std::unordered_map<uint8_t, const char *> GSBrush2Drums =
{
};

// GS Drum Set 43 "Brush 2 L/R", Roland SC-8850 Owner's Manual, p. 192
const std::unordered_map<uint8_t, const char *> GSBrush2LRDrums =
{
};

// GS Drum Set 49 "Orchestra", Roland SC-8850 Owner's Manual, p. 192
const std::unordered_map<uint8_t, const char *> GSOrchestraDrums =
{
    { 26, "Finger Snap" },
    { 27, "Closed Hi-Hat 2" },
    { 28, "Pedal Hi-Hat" },
    { 29, "Open Hi-Hat 2" },
    { 30, "Ride Cymbal 1" },

    { 35, "Jazz Kick 1" },
    { 36, "Concert Bass Drum 1" },

    { 38, "Concert Snare Drum" },
    { 39, "Castanets" },
    { 40, "Concert Snare Drum" },
    { 41, "Timpani F" },
    { 42, "Timpani F#" },
    { 43, "Timpani G" },
    { 44, "Timpani G#" },
    { 45, "Timpani A" },
    { 46, "Timpani A#" },
    { 47, "Timpani B" },
    { 48, "Timpani C" },
    { 49, "Timpani C#" },
    { 50, "Timpani D" },
    { 51, "Timpani D#" },
    { 52, "Timpani E" },
    { 53, "Timpani F" },

    { 57, "Concert Cymbal 2" },

    { 59, "Concert Cymbal 1" },

    { 88, "Applause" },
};

// GS Drum Set 50 "Ethnic", Roland SC-8850 Owner's Manual, p. 192
const std::unordered_map<uint8_t, const char *> GSEthnicDrums =
{
    { 25, "Finger Snap" },
    { 26, "Tambourine" },
    { 27, "Castanets" },
    { 28, "Crash Cymbal 1" },
    { 29, "Snare Roll" },
    { 30, "Concert Snare Drum" },
    { 31, "Concert Cymbal" },
    { 32, "Convert Bass Drum 1" },
    { 33, "Jingle Bell" },
    { 34, "Bell Tree" },
    { 35, "Bar Chimes" },
    { 36, "Wadaiko" },
    { 37, "Wadaiko Rim" },
    { 38, "Shime Taiko" },
    { 39, "Atarigane" },
    { 40, "Hyoushigi" },
    { 41, "Ohkawa" },
    { 42, "High kotsuzumi" },
    { 43, "Low kotsuzumi" },
    { 44, "Ban Gu" },
    { 45, "Big Gong" },
    { 46, "Small Gong" },
    { 47, "Bend Gong" },
    { 48, "Thai Gong" },
    { 49, "Rama Cymbal" },
    { 50, "Gamelan Gong" },
    { 51, "Udo Short" },
    { 52, "Udo Long" },
    { 53, "Udo Slap" },
    { 54, "Bendir" },
    { 55, "Req Drum" },
    { 56, "Req Tik" },
    { 57, "Tabla Te" },
    { 58, "Tabla Na" },
    { 59, "Tabla Tun" },
    { 60, "Tabla Ge" },
    { 61, "Tabla Ge Hi" },
    { 62, "Talking Drum" },
    { 63, "Bend Talking Drum" },
    { 64, "Caxixi" },
    { 65, "Djembe" },
    { 66, "Djembe Rim" },
    { 67, "Timbales Low" },
    { 68, "Timbales Paila" },
    { 69, "Timbales High" },
    { 70, "Cowbell" },
    { 71, "High Bongo" },
    { 72, "Low Bongo" },
    { 73, "Mute High Conga" },
    { 74, "Open High Conga" },
    { 75, "Mute Low Conga" },
    { 76, "Conga Slap" },
    { 77, "Open Low CongaBlock" },
    { 78, "Conga Slide" },
    { 79, "Mute Pandiero" },
    { 80, "Open Pandiero" },
    { 81, "Open Surdo" },
    { 82, "Mute Surdo" },

    { 83, "Tamborim" },
    { 84, "High Agogo" },
    { 85, "Low Agogo" },
    { 86, "Shaker" },
    { 87, "High Whistle" },
    { 88, "Low Whistle" },
    { 89, "Mute Cuica" },
    { 90, "Open Cuica" },
    { 91, "Mute Triangle" },
    { 92, "Open Triangle" },
    { 93, "Short Guiro" },
    { 94, "Long Guiro" },
    { 95, "Cabasa Up" },
    { 96, "Cabasa Down" },
};

// GS Drum Set 51 "Kick & Snare", Roland SC-8850 Owner's Manual, p. 192
const std::unordered_map<uint8_t, const char *> GSKicknSnareDrums =
{
};

// GS Drum Set 52 "Kick & Snare 2", Roland SC-8850 Owner's Manual, p. 193
const std::unordered_map<uint8_t, const char *> GSKicknSnare2Drums =
{
};

// GS Drum Set 53 "Asia", Roland SC-8850 Owner's Manual, p. 193
const std::unordered_map<uint8_t, const char *> GSAsiaDrums =
{
};

// GS Drum Set 54 "Cymbal & Claps", Roland SC-8850 Owner's Manual, p. 193
const std::unordered_map<uint8_t, const char *> GSCymbalnClapsDrums =
{
};

// GS Drum Set 55 "Gamelan 1", Roland SC-8850 Owner's Manual, p. 193
const std::unordered_map<uint8_t, const char *> GSGamelan1Drums =
{
};

// GS Drum Set 56 "Gamelan 2", Roland SC-8850 Owner's Manual, p. 193
const std::unordered_map<uint8_t, const char *> GSGamelan2Drums =
{
};

// GS Drum Set 57 "SFX", Roland SC-8850 Owner's Manual, p. 194
const std::unordered_map<uint8_t, const char *> GSSFXDrums =
{
    { 22, "MC-500 Beep 2" },
    { 23, "Guitar Slide" },
    // C1
    { 24, "Guitar Wah" },
    { 25, "Guitar Slap" },
    { 26, "Chord Stroke Down" },
    { 27, "Chord Stroke Up" },
    { 28, "Biwa FX" },
    { 29, "Phonograph Noise" },
    { 30, "Tape Rewind" },
    { 31, "Scratch Push 2" },
    { 32, "Scratch Pull 2" },
    { 33, "Cutting Noise 2 Up" },
    { 34, "Cutting Noise 2 Down" },
    { 35, "Distortion Guitar Cutting Noise Up" },
    // C2
    { 36, "Distortion Guitar Cutting Noise Up" },
    { 37, "Bass Slide" },
    { 38, "Pick Scrape" },
    { 39, "High Q" },
    { 40, "Slap" },
    { 41, "Scratch Push" },
    { 42, "Scratch PUll" },
    { 43, "Sticks" },
    { 44, "Square Click" },
    { 45, "Metronome Click" },
    { 46, "Metronome Bell" },
    { 47, "Guitar Fret Noise" },
    // C3
    { 48, "Guitar Cutting Noise Up" },
    { 49, "Guitar Cutting Noise Down" },
    { 50, "String Slap of Double Bass" },
    { 51, "Flute Key Click Noise" },
    { 52, "Laughing" },
    { 53, "Screaming" },
    { 54, "Punch" },
    { 55, "Heart Beat" },
    { 56, "Footsteps 1" },
    { 57, "Footsteps 2" },
    { 58, "Applause" },
    { 59, "Door Creaking" },
    // C4
    { 60, "Door" },
    { 61, "Scratch" },
    { 62, "Wind Chimes" },
    { 63, "Car - Engine" },
    { 64, "Car - Stop" },
    { 65, "Car - Pass" },
    { 66, "Car - Crash" },
    { 67, "Siren" },
    { 68, "Train" },
    { 69, "Jetplane" },
    { 70, "Helicopter" },
    { 71, "Starship" },
    // C5
    { 72, "Gun Shot" },
    { 73, "Machine Gun" },
    { 74, "Laser Gun" },
    { 75, "Explosion" },
    { 76, "Dog" },
    { 77, "Horse-Gallop" },
    { 78, "Birds" },
    { 79, "Rain" },
    { 80, "Thunder" },
    { 81, "Wind" },
    { 82, "Seashore" },
    { 83, "Stream" },
    // C6
    { 84, "Bubble" },
    { 85, "Kitty" },
    { 86, "Bird 2" },
    { 87, "Growl" },

    { 89, "Telephone 1" },
    { 90, "Telephone 2" },
    { 91, "Small Club 1" },
    { 92, "Small Club 2" },
    { 93, "Applause Wave" },
    { 94, "Eruption" },
    { 95, "Big Shot" },
    // C7
    { 96, "Percussion Bang" },
};

// GS Drum Set 58 "Rhythm FX", Roland SC-8850 Owner's Manual, p. 194
const std::unordered_map<uint8_t, const char *> GSRhythmFXDrums =
{
};

// GS Drum Set 59 "Rhythm FX 2", Roland SC-8850 Owner's Manual, p. 194
const std::unordered_map<uint8_t, const char *> GSRhythmFX2Drums =
{
};

// GS Drum Set 60 "Rhythm FX 3", Roland SC-8850 Owner's Manual, p. 194
const std::unordered_map<uint8_t, const char *> GSRhythmFX3Drums =
{
};

// GS Drum Set 61 "SFX 2", Roland SC-8850 Owner's Manual, p. 195
const std::unordered_map<uint8_t, const char *> GSSFX2Drums =
{
    { 31, "Acoustic Bass Mute Noise" },
    { 32, "Acoustic Bass Touch Noise" },
    { 33, "Acoustic Bass Attack Noise" },
    { 34, "Distortion Guitar Mute Noise" },
    { 35, "Steel Guitar Slide Noise 1" },
    // C2
    { 36, "Steel Guitar Slide Noise 2" },
    { 37, "Steel Guitar Slide Noise 3" },
    { 38, "Steel Guitar Slide Noise 4" },
    { 39, "Guitar Stroke Noise 1" },
    { 40, "Guitar Stroke Noise 2" },
    { 41, "Guitar Stroke Noise 3" },
    { 42, "Guitar Stroke Noise 4" },
    { 43, "Guitar Stroke Noise 5" },
    { 44, "Open CD Tray" },
    { 45, "Audio Switch" },
    { 46, "Keyboard Typing 1" },
    { 47, "Keyboard Typing 2" },
    // C3
    { 48, "Keyboard Typing 3" },
    { 49, "Keyboard Typing 4" },
    { 50, "Keyboard Typing 5" },
    { 51, "Keyboard Typing 6" },
    { 52, "Baby Laughing" },
    { 53, "Clap Hit" },
    { 54, "Stab! 1" },
    { 55, "Stab! 2" },
    { 56, "Bounce Hit" },
    { 57, "Boeeeen" },
    { 58, "Glass Stir" },
    { 59, "Ice Ring" },
    // C4
    { 60, "Crack Bottle" },
    { 61, "Pour Bottle" },
    { 62, "Soda" },
    { 63, "Car - Engine 2" },
    { 64, "Car - Horn" },
    { 65, "Railroad Crossing" },
    { 66, "SL 1" },
    { 67, "SL 2" },
    { 68, "Over Blow" },
    { 69, "Sword Boom!" },
    { 70, "Sword Cross" },
    { 71, "Industry Hit" },
    // C5
    { 72, "Drill Hit" },
    { 73, "Compresser" },
    { 74, "Thrill Hit" },
    { 75, "Explosion 2" },
    { 76, "Seal" },
    { 77, "Fancy Animal" },
    { 78, "Cricket" },
    { 79, "Bear" },
    { 80, "Frog Voice" },
    { 81, "Wind 2" },
    { 82, "Scratch 3" },
    { 83, "Scratch 4" },
    // C6
    { 84, "Scratch 5" },
    { 85, "Scratch 6" },
    { 86, "Scratch 7" },
    { 87, "Noise Attack" },
    { 88, "Bounce" },
    { 89, "Dist Knock" },
    { 90, "Bound" },
};

// GS Drum Set 62 "Voice", Roland SC-8850 Owner's Manual, p. 195
const std::unordered_map<uint8_t, const char *> GSVoiceDrums =
{
};

// GS Drum Set 63 "Cym & Claps 2", Roland SC-8850 Owner's Manual, p. 195
const std::unordered_map<uint8_t, const char *> GSCymbalnClaps2Drums =
{
};

// GS Drum Set 128 "CM-64/CM-32L", Roland SC-88Pro Owner's Manual, p. 175
const std::unordered_map<uint8_t, const char *> GSCM64_32LDrums =
{
    {  35, "CM Kick Drum" },
    {  36, "CM Kick Drum" },
    {  37, "CM Rim Shot" },
    {  38, "CM Snare Drum" },
    {  39, "CM Hand Clap" },
    {  40, "CM Electric Snare Drum" },
    {  41, "CM Acoustic Low Tom" },
    {  42, "CM Closed High Hat" },
    {  43, "CM Acoustic Low Tom" },
    {  44, "CM Open High Hat 2" },
    {  45, "CM Acoustic Middle Tom" },
    {  46, "CM Open High Hat 1" },
    {  47, "CM Acoustic Middle Tom" },
    {  48, "CM Acoustic High Tom" },
    {  49, "CM Crash Cymbal" },
    {  50, "CM Acoustic High Tom" },
    {  51, "CM Ride Cymbal" },

    {  54, "CM Tambourine" },
    
    {  56, "CM Cowbell" },

    {  60, "CM High Bongo" },
    {  61, "CM Low Bongo" },
    {  62, "CM Mute High Conga" },
    {  63, "CM High Conga" },
    {  64, "CM Low Conga" },
    {  65, "CM High Timbale" },
    {  66, "CM Low Timbale" },
    {  67, "CM High Agogo" },
    {  68, "CM Low Agogo" },
    {  69, "CM Cabasa" },
    {  70, "CM Maracas" },
    {  71, "CM Short Whistle" },
    {  72, "CM Long Whistle" },
    {  73, "CM Vibrato Slap" },

    {  75, "CM Claves" },
    {  76, "Laughing" },
    {  77, "Scream" },
    {  78, "Punch" },
    {  79, "Heart Beat" },
    {  80, "Footsteps 1" },
    {  81, "Footsteps 2" },
    {  82, "Applause" },
    {  83, "Creaking" },
    {  84, "Door" },
    {  85, "Scratch" },
    {  86, "Wind Chimes" },

    {  87, "Car - Engine" },
    {  88, "Car - Stop" },
    {  89, "Car - Pass" },
    {  90, "Car - Crash" },
    {  91, "Siren" },
    {  92, "Train" },
    {  93, "Jetplane" },
    {  94, "Helicopter" },
    {  95, "Starship" },
    {  96, "Gun Shot" },
    {  97, "Machine Gun" },
    {  98, "Laser Gun" },
    {  99, "Explosion" },
    { 100, "Dog" },
    { 101, "Horse Gallop" },
    { 102, "Birds" },
    { 103, "Rain" },
    { 104, "Thunder" },
    { 105, "Wind" },
    { 106, "Seashore" },
    { 107, "Stream" },
    { 108, "Bubble" },
};

/// <summary>
/// Gets the name of the specified drum set.
/// </summary>
std::string DescribeDrumSet(uint8_t drumSet, bool isGS) noexcept
{
    const auto & DrumSets = isGS ? GSDrumSets : GM1DrumSets;

    const auto & it = DrumSets.find(drumSet);
    
    if (it != DrumSets.end())
        return it->second;

    return "Unknown drum set";
}

/// <summary>
/// Gets the name of the instrument in the specified drum set.
/// </summary>
std::string DescribeDrum(uint8_t drumSet, uint8_t noteNumber, bool isGS) noexcept
{
    if (isGS)
    {
        const std::map<uint8_t, const std::unordered_map<uint8_t, const char *> *> DrumSets =
        {
            {   0, &GSStandard1Drums },
            {   1, &GSStandard2Drums },
            {   2, &GSStandardLRDrums },

            {   8, &GSRoomDrums },
            {   9, &GSHipHopDrums },
            {  10, &GSJungleDrums },
            {  11, &GSTechnoDrums },
            {  12, &GSRoomLRDrums },
            {  13, &GSHouseDrums },

            {  16, &GSPowerDrums },

            {  24, &GSElectronicDrums },
            {  25, &GSTR808Drums },
            {  26, &GSDanceDrums },
            {  27, &GSCR78Drums },
            {  28, &GSTR606Drums },
            {  29, &GSTR707Drums },
            {  30, &GSTR909Drums },

            {  32, &GSJazzDrums },
            {  33, &GSJazzLRDrums },

            {  40, &GSBrushDrums },
            {  41, &GSBrush2Drums },
            {  42, &GSBrush2LRDrums },

            {  48, &GSOrchestraDrums },
            {  49, &GSEthnicDrums },
            {  50, &GSKicknSnareDrums },
            {  51, &GSKicknSnare2Drums },
            {  52, &GSAsiaDrums },
            {  53, &GSCymbalnClapsDrums },
            {  54, &GSGamelan1Drums },
            {  55, &GSGamelan2Drums },

            {  56, &GSSFXDrums },
            {  57, &GSRhythmFXDrums },
            {  58, &GSRhythmFX2Drums },
            {  59, &GSRhythmFX3Drums },
            {  60, &GSSFX2Drums },
            {  61, &GSVoiceDrums },
            {  62, &GSCymbalnClaps2Drums },

            { 127, &GSCM64_32LDrums },
        };

        std::string InstrumentName;

        for (const auto & DrumSetIter : DrumSets)
        {
            if (DrumSetIter.first > drumSet)
                break;

            const auto & NoteIter = DrumSetIter.second->find(noteNumber);

            if (NoteIter != DrumSetIter.second->end())
                InstrumentName = msc::FormatText("GS drum set %d, %s", drumSet + 1, NoteIter->second);
        }

        if (!InstrumentName.empty())
            return InstrumentName;

        return msc::FormatText("GS drum set %d, Unknown drum instrument %d", drumSet + 1, noteNumber);
    }
    else
    {
        const auto & NoteIter = GMStandardDrums.find(noteNumber);
    
        if (NoteIter != GMStandardDrums.end())
            return msc::FormatText("GM1 drum set, %s", NoteIter->second);

        return msc::FormatText("GM1 drum set, Unknown drum instrument %d", noteNumber);
    }
}

/// <summary>
/// Describes a Control Change message.
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
        case 0x06: ::snprintf(Line.data(), Line.size(), "Data Entry (MSB) %d", Value); break;                                                      // Sets the value for NRPN or RPN parameters. (GM2)
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
        case 0x26: ::snprintf(Line.data(), Line.size(), "Data Entry (LSB) %d", Value); break;                                                      // Sets the value for NRPN or RPN parameters. (GM2)
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

        case 0x60: ::snprintf(Line.data(), Line.size(), "Data Increment %d", Value); break;
        case 0x61: ::snprintf(Line.data(), Line.size(), "Data Decrement %d", Value); break;

        case 0x62: ::snprintf(Line.data(), Line.size(), "Select NRPN parameter (LSB)"); break;
        case 0x63: ::snprintf(Line.data(), Line.size(), "Select NRPN parameter (MSB)"); break;

        case 0x64: ::snprintf(Line.data(), Line.size(), "Select RPN parameter (LSB)"); break;                                                       // (GM2)
        case 0x65: ::snprintf(Line.data(), Line.size(), "Select RPN parameter (MSB)"); break;                                                       // (GM2)

        case 0x6E: ::snprintf(Line.data(), Line.size(), "Loop marker (%d %d)", (int) d1, Value); break;
        case 0x6F: ::snprintf(Line.data(), Line.size(), "Loop marker (%d %d)", (int) d1, Value); break;

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
