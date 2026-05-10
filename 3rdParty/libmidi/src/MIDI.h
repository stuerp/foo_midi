
/** $VER: MIDI.h (2026.05.09) **/

#pragma once

namespace midi
{

enum StatusCode : uint8_t
{
    NoteOff                 = 0x80,
    NoteOn                  = 0x90,

    KeyPressure             = 0xA0, // Polyphonic Key Pressure (Aftertouch)
    ControlChange           = 0xB0,
    ProgramChange           = 0xC0,
    ChannelPressure         = 0xD0,
    PitchBendChange         = 0xE0,

    SysEx                   = 0xF0,
    MIDITimeCodeQtrFrame    = 0xF1,
    SongPositionPointer     = 0xF2,
    SongSelect              = 0xF3,

    TuneRequest             = 0xF6,
    SysExEnd                = 0xF7,

    // Real-time events
    TimingClock             = 0xF8,

    Start                   = 0xFA,
    Continue                = 0xFB,
    Stop                    = 0xFC,

    ActiveSensing           = 0xFE,
    MetaData                = 0xFF
};

enum Controller : uint8_t
{
    BankSelect              = 0x00, // CC   0, Switch bank for patch selection
    Modulation              = 0x01, // CC   1, Controls a vibrato effect (pitch, loudness, brighness). What is modulated depends on the program.
    BreathController        = 0x02, // CC   2, Originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.
    //                        0x03  // CC   3, Undefined
    FootController          = 0x04, // CC   4, Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
    PortamentoTime          = 0x05, // CC   5 (GM2)
    DataEntry               = 0x06, // CC   6, Sets the Value for NRPN or RPN parameters.
    ChannelVolume           = 0x07, // CC   7
    Balance                 = 0x08, // CC   8, Controls the left and right balance, generally for stereo patches. 0 = hard left, 64 = center, 127 = hard right.
    //                        0x09  // CC   9, Undefined
    Pan                     = 0x0A, // CC  10
    Expression              = 0x0B, // CC  11
    EffectControl1          = 0x0C, // CC  12, Usually used to control a parameter of an effect within the synth/workstation.
    EffectControl2          = 0x0D, // CC  13, Usually used to control a parameter of an effect within the synth/workstation.
    //                        0x0E  // CC  14, Undefined
    //                        0x0F  // CC  15, Undefined

    GeneralPurpose1         = 0x10, // CC  16
    GeneralPurpose2         = 0x11, // CC  17
    GeneralPurpose3         = 0x12, // CC  18
    GeneralPurpose4         = 0x13, // CC  19

    // LSB for CC 0 to 31
    BankSelectLSB           = 0x20, // CC  32

    DataEntryLSB            = 0x26, // CC  38, Sets the Value for NRPN or RPN parameters.
    ChannelVolumeLSB        = 0x27, // CC  39

    PanLSB                  = 0x2A, // CC  42
    ExpressionLSB           = 0x2B, // CC  43

    DamperPedal             = 0x40, // CC  64, On/Off switch that controls sustain. (See also Sostenuto CC 66) 0 to 63 = Off, 64 to 127 = On
    Portamento              = 0x41, // CC  65, On/Off switch 0 to 63 = Off, 64 to 127 = On (GM2)
    Sostenuto               = 0x42, // CC  66, On/Off switch. Like the Sustain controller (CC 64), However it only holds notes that were “On” when the pedal was pressed. People use it to “hold” chords” and play melodies over the held chord. 0 to 63 = Off, 64 to 127 = On (GM2)
    SoftPedal               = 0x43, // CC  67, Lowers the volume of notes played. 0 to 63 = Off, 64 to 127 = On (GM2)
    LegatoFootswitch        = 0x44, // CC  68, Turns Legato effect between 2 subsequent notes On or Off. 0 to 63 = Off, 64 to 127 = On
    Hold2                   = 0x45, // CC  69, Another way to “hold notes” (see MIDI CC 64 and MIDI CC 66). However notes fade out according to their release parameter rather than when the pedal is released. 0 to 63 = Off, 64 to 127 = On (GM2)

    SoundController1        = 0x46, // CC  70, Default = Sound Variation
    SoundController2        = 0x47, // CC  71, Default = Timbre / Harmonic Intensity (GM2)
    SoundController3        = 0x48, // CC  72, Default = Release Time (GM2)
    SoundController4        = 0x49, // CC  73, Default = Attack Time (GM2)
    SoundController5        = 0x4A, // CC  74, Default = Brightness (GM2)
    SoundController6        = 0x4B, // CC  75, Generic
    SoundController7        = 0x4C, // CC  78, Generic
    SoundController8        = 0x4D, // CC  77, Generic
    SoundController9        = 0x4E, // CC  78, Generic
    SoundController10       = 0x4F, // CC  79, Generic

    GeneralPurpose5         = 0x50, // CC  80
    GeneralPurpose6         = 0x51, // CC  81
    GeneralPurpose7         = 0x52, // CC  82
    GeneralPurpose8         = 0x53, // CC  83

    PortamentoControl       = 0x54, // CC  84, Controls portamento rate to slide between 2 notes played subsequently.

    //                        0x55  // CC  85, Undefined
    //                        0x56  // CC  86, Undefined
    //                        0x57  // CC  87, Undefined
    //                        0x58  // CC  88, Undefined
    //                        0x59  // CC  89, Undefined
    //                        0x5A  // CC  90, Undefined

    EffectDepth1            = 0x5B, // CC  91, Effect 1 Depth (GM2)
    EffectDepth2            = 0x5C, // CC  92, Effect 2 Depth (GM2)
    EffectDepth3            = 0x5D, // CC  93, Effect 3 Depth (GM2)
    EffectDepth4            = 0x5E, // CC  94, Effect 4 Depth (GM2)
    EffectDepth5            = 0x5F, // CC  95, Effect 5 Depth (GM2)

    DataIncrement           = 0x60, // CC  96, Increment the value of the selected RPN or NRPN parameter.
    DataDecrement           = 0x61, // CC  97, Increment the value of the selected RPN or NRPN parameter.

    NRPNLSB                 = 0x62, // CC  98, Select NRPN parameter (LSB)
    NRPNMSB                 = 0x63, // CC  99, Select NRPN parameter (MSB)

    RPNLSB                  = 0x64, // CC 100, Select RPN parameter (LSB)
    RPNMSB                  = 0x65, // CC 101, Select RPN parameter (MSB)

    // Channel Mode messages
    AllSoundsOff            = 0x78, // CC 120, Silences all notes current sounding on the specified MIDI channel. Upon receiving the message, all notes should be turned off, and the output set to zero as quickly as possible.
    ResetAllControllers     = 0x79, // CC 121, When the data byte is   0, the device will reset all controllers to their default values except the following controllers: Volume (Controller 7) and Pan Control (Controller 10)
                                    //         When the data byte is 127, the device will reset all controllers to their power-on default values. This should include all MIDI controllers and RPNs. It is left to the discretion of the device designer whether to reset any NRPNs used by the device. However, the behavior must be documented.
    LocalControl            = 0x7A, // CC 122, Disables or enables internal connection of a MIDI keyboard/workstation, etc.
    AllNotesOff             = 0x7B, // CC 123, Performs a Note-Off event for all notes on the specified MIDI channel. If the Sustain Pedal (MIDI Controller 64) is active, the notes should continue to sustain until a Sustain Pedal release event is sent.
    OmniOff                 = 0x7C, // CC 124, Sets to “Omni Off” mode.
    OmniOn                  = 0x7D, // CC 125, Sets to “Omni On” mode.
    MonoOn                  = 0x7E, // CC 126, Sets device mode to Monophonic.
    PolyOn                  = 0x7F, // CC 127, Sets device mode to Polyphonic.
};

enum MetaDataType : uint8_t
{
    SequenceNumber          = 0x00, // Sequence number in type 0 and 1 MIDI files; pattern number in type 2 MIDI files. (0..65535, default 0, occurs at delta time 0)
    Text                    = 0x01, // General "Text" Meta Message. Can be used for any text based data. (string)
    Copyright               = 0x02, // Provides information about a MIDI file’s copyright. (string, occurs at delta time 0 in the first track)
    TrackName               = 0x03, // Track name (string, occurs at delta time 0)
    InstrumentName          = 0x04, // Instrument name (string)
    Lyrics                  = 0x05, // Stores the lyrics of a song. Typically one syllable per Meta Message. (string)
    Marker                  = 0x06, // Marks a point of interest in a MIDI file. Can be used as the marker for the beginning of a verse, solo, etc. (string)
    CueMarker               = 0x07, // Marks a cue. IE: ‘Cue performer 1’, etc (string)

    DeviceName              = 0x09, // Gives the name of the device. (string)

    ChannelPrefix           = 0x20, // Gives the prefix for the channel on which events are played. (0..255, default 0)
    MIDIPort                = 0x21, // Gives the MIDI Port on which events are played. (0..255, default 0)

    EndOfTrack              = 0x2F, // An empty Meta Message that marks the end of a track. Occurs at the end of each track.

    SetTempo                = 0x51, // Tempo is in microseconds per beat (quarter note). (0..16777215, default 500000 μs per beat / 120 beats per minute). Occurs anywhere but usually in the first track.

    SMPTEOffset             = 0x54, // SMPTE time to denote playback offset from the beginning. Occurs at the beginning of a track and in the first track of files with MIDI format type 1.

    TimeSignature           = 0x58, // 
    KeySignature            = 0x59, // Valid values: A A#m Ab Abm Am B Bb Bbm Bm C C# C#m Cb Cm D D#m Db Dm E Eb Ebm Em F F# F#m Fm G G#m Gb Gm

    SequencerSpecific       = 0x7F  // An unprocessed sequencer specific message containing raw data.
};

/// <summary>
/// Creates a MIDI message by packing the status code, data bytes and port number into a single 32-bit integer.
/// </summary>
inline uint32_t PackMessage(uint8_t statusCode, uint8_t data1, uint8_t data2, uint8_t portNumber) noexcept
{
    return (uint32_t) ((portNumber << 24) | (data2 << 16) | (data1 << 8) | statusCode);
}

/// <summary>
/// Converts  a pitch bend value (-8192 to +8191) to two MIDI data bytes (LSB, MSB).
/// </summary>
inline std::pair<uint8_t, uint8_t> PitchBendToBytes(int32_t value) noexcept
{
    // Clamp to valid 14-bit range.
    value = msc::Clamp(value, -8192, 8191);

    const auto Value = (uint16_t) (value + 8192);

    const auto LSB = (uint8_t)  (Value       & 0x7F); // Lower 7 bits
    const auto MSB = (uint8_t) ((Value >> 7) & 0x7F); // Upper 7 bits

    return { LSB, MSB };
}

// Convert two MIDI data bytes back to pitch bend value (-8192 to +8191)
inline int32_t BytesToPitchBend(uint8_t lsb, uint8_t msb) noexcept
{
    int32_t Value = ((uint16_t) msb << 7) | lsb;

    return Value - 8192;
}

}
