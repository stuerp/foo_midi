typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned Uint32;

extern const struct adldata
{
    Uint32 modulator_E862, carrier_E862;  // See below
    Uint8 modulator_40, carrier_40; // KSL/attenuation settings
    Uint8 feedconn; // Feedback/connection bits for the channel

    signed char finetune;
} adl[];
extern const struct adlinsdata
{
    enum { Flag_Pseudo4op = 0x01, Flag_NoSound = 0x02 };

    Uint16 adlno1, adlno2;
    Uint8 tone;
    Uint8 flags;
    signed char p4o_second_offset; // offset pitch of second voice, 64 = one semitone
    Uint16 ms_sound_kon;  // Number of milliseconds it produces sound;
    Uint16 ms_sound_koff;
} adlins[];
extern const unsigned short banks[][256];
extern const char* const banknames[67];
