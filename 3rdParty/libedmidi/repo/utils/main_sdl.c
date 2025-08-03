#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <SDL2/SDL.h>

#include <emu_de_midi.h>

/* prototype for our audio callback */
/* see the implementation for more information */
static void my_audio_callback(void *midi_player, Uint8 *stream, int len);

/* variable declarations */
static SDL_bool s_is_playing = SDL_FALSE; /* remaining length of the sample we have to play */

static struct EDMIDI_AudioFormat s_audioFormat;

static void stop_playing(int sig)
{
    (void)sig;
    s_is_playing = SDL_FALSE;
}

#if defined(__WATCOMC__)
#include <stdio.h> // snprintf is here!
#define flushout(stream)
#else
#define flushout(stream) fflush(stream)
#endif

const char* audio_format_to_str(int format)
{
    switch(format)
    {
    case AUDIO_S8:
        return "S8";
    case AUDIO_U8:
        return "U8";
    case AUDIO_S16MSB:
        return "S16MSB";
    case AUDIO_S16LSB:
        return "S16LSB";
    case AUDIO_U16LSB:
        return "U16LSB";
    case AUDIO_U16MSB:
        return "U16MSB";
    case AUDIO_S32MSB:
        return "S32MSB";
    case AUDIO_S32LSB:
        return "S32LSB";
    case AUDIO_F32MSB:
        return "F32MSB";
    case AUDIO_F32LSB:
        return "F32LSB";
    }
    return "UNK";
}

static void debugPrint(void *userdata, const char *fmt, ...)
{
    char buffer[4096];
    va_list args;
    int rc;

    (void)userdata;

    va_start(args, fmt);
    rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if(rc > 0)
    {
        fprintf(stdout, " - Debug: %s\n", buffer);
        flushout(stdout);
    }
}

static inline void secondsToHMSM(double seconds_full, char *hmsm_buffer, size_t hmsm_buffer_size)
{
    double seconds_integral;
    double seconds_fractional = modf(seconds_full, &seconds_integral);
    unsigned int milliseconds = (unsigned int)(seconds_fractional * 1000.0);
    unsigned int seconds = (unsigned int)(SDL_fmod(seconds_full, 60.0));
    unsigned int minutes = (unsigned int)(SDL_fmod(seconds_full / 60, 60.0));
    unsigned int hours   = (unsigned int)(seconds_full / 3600);

    SDL_memset(hmsm_buffer, 0, hmsm_buffer_size);
    if (hours > 0)
        sprintf(hmsm_buffer, "%02u:%02u:%02u,%03u", hours, minutes, seconds, milliseconds);
    else
        sprintf(hmsm_buffer, "%02u:%02u,%03u", minutes, seconds, milliseconds);
}


int main(int argc, char **argv)
{
    /* local variables */
    static SDL_AudioSpec            spec, obtained; /* the specs of our piece of music */
    static struct EDMIDIPlayer      *midi_player = NULL; /* Instance of Emu De Midi player */
    static const char               *music_path = NULL; /* Path to music file */
    SDL_bool loopEnabled = SDL_TRUE;
    char totalHMS[25];
    char loopStartHMS[25];
    char loopEndHMS[25];
    char posHMS[25];
    double loopStart, loopEnd, total;
    uint64_t milliseconds_prev = ~0u;
    int printsCounter = 0;
    int printsCounterPeriod = 1;
    double time_pos;
    uint64_t milliseconds;
    int songNumLoad = -1;

    SDL_memset(posHMS, 0, 25);
    SDL_memset(totalHMS, 0, 25);
    SDL_memset(loopStartHMS, 0, 25);
    SDL_memset(loopEndHMS, 0, 25);


    SDL_memset(&spec, 0, sizeof(SDL_AudioSpec));
    spec.freq = 44100;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 512;

    /* set the callback function */
    spec.callback = my_audio_callback;
    /* set ADLMIDI's descriptor as userdata to use it for sound generation */
    spec.userdata = &midi_player;

    signal(SIGINT, stop_playing);
    signal(SIGTERM, stop_playing);

    fprintf(stdout, "==========================================\n"
                    "         libEDMIDI demo utility\n"
                    "==========================================\n\n");
    flushout(stdout);

    if (argc < 2)
    {
        fprintf(stderr, "\n"
                        "\n"
                        "No given files to play!\n"
                        "\n"
                        "Syntax: %s <path-to-MIDI-file>\n"
                        "\n", argv[0]);
        return 2;
    }

    music_path = argv[1];

    if(argc >= 3)
        songNumLoad = strtol(argv[2], NULL, 10);

    fprintf(stdout, " - Library version %s\n", edmidi_linkedLibraryVersion());

    /* Initialize SDL.*/
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        flushout(stderr);
        return 1;
    }

    /* Open the audio device */
    if(SDL_OpenAudio(&spec, &obtained) < 0)
    {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        flushout(stderr);
        edmidi_close(midi_player);
        SDL_Quit();
        return 1;
    }

    fprintf(stdout, " - Audio output: format=%s, samples=%u, freq=%u, channels=%u\n",
            audio_format_to_str(obtained.format),
            obtained.samples,
            obtained.freq,
            obtained.channels);
    flushout(stdout);

    switch(obtained.format)
    {
    case AUDIO_S8:
        s_audioFormat.type = EDMIDI_SampleType_S8;
        s_audioFormat.containerSize = sizeof(int8_t);
        s_audioFormat.sampleOffset = sizeof(int8_t) * 2;
        break;
    case AUDIO_U8:
        s_audioFormat.type = EDMIDI_SampleType_U8;
        s_audioFormat.containerSize = sizeof(uint8_t);
        s_audioFormat.sampleOffset = sizeof(uint8_t) * 2;
        break;
    case AUDIO_S16:
    case AUDIO_S16MSB:
        s_audioFormat.type = EDMIDI_SampleType_S16;
        s_audioFormat.containerSize = sizeof(int16_t);
        s_audioFormat.sampleOffset = sizeof(int16_t) * 2;
        break;
    case AUDIO_U16:
    case AUDIO_U16MSB:
        s_audioFormat.type = EDMIDI_SampleType_U16;
        s_audioFormat.containerSize = sizeof(uint16_t);
        s_audioFormat.sampleOffset = sizeof(uint16_t) * 2;
        break;
    case AUDIO_S32:
    case AUDIO_S32MSB:
        s_audioFormat.type = EDMIDI_SampleType_S32;
        s_audioFormat.containerSize = sizeof(int32_t);
        s_audioFormat.sampleOffset = sizeof(int32_t) * 2;
        break;
    case AUDIO_F32:
    case AUDIO_F32MSB:
        s_audioFormat.type = EDMIDI_SampleType_F32;
        s_audioFormat.containerSize = sizeof(float);
        s_audioFormat.sampleOffset = sizeof(float) * 2;
        break;
    default:
        fprintf(stderr, "Unsupported output audio format: %d\n", obtained.format);
        flushout(stderr);
        SDL_CloseAudio();
        SDL_Quit();
        return 1;
    }

    /* Initialize EmuDeMidi */
    midi_player = edmidi_initEx(obtained.freq, 2);
    if (!midi_player)
    {
        fprintf(stderr, "Couldn't initialize EDMIDI: %s\n", edmidi_errorString());
        flushout(stderr);
        SDL_CloseAudio();
        SDL_Quit();
        return 1;
    }

    if(songNumLoad >= 0)
        edmidi_selectSongNum(midi_player, songNumLoad);

    //Set internal debug messages hook to print all libADLMIDI's internal debug messages
    edmidi_setDebugMessageHook(midi_player, debugPrint, NULL);

    /* Open the MIDI (or MUS, IMF or CMF) file to play */
    if (edmidi_openFile(midi_player, music_path) < 0)
    {
        fprintf(stderr, "Couldn't open music file: %s\n", edmidi_errorInfo(midi_player));
        flushout(stderr);
        edmidi_close(midi_player);
        SDL_CloseAudio();
        SDL_Quit();
        return 1;
    }

    edmidi_setLoopEnabled(midi_player, loopEnabled);

    total               = edmidi_totalTimeLength(midi_player);
    loopStart           = edmidi_loopStartTime(midi_player);
    loopEnd             = edmidi_loopEndTime(midi_player);

    secondsToHMSM(total, totalHMS, 25);
    if(loopStart >= 0.0 && loopEnd >= 0.0)
    {
        secondsToHMSM(loopStart, loopStartHMS, 25);
        secondsToHMSM(loopEnd, loopEndHMS, 25);
    }

    fprintf(stdout, " - Track count: %lu\n", (unsigned long)(edmidi_trackCount(midi_player)));

    fprintf(stdout, " - Loop is turned %s\n", loopEnabled ? "ON" : "OFF");
    if(loopStart >= 0.0 && loopEnd >= 0.0)
        fprintf(stdout, " - Has loop points: %s ... %s\n", loopStartHMS, loopEndHMS);

    int songsCount = edmidi_getSongsCount(midi_player);
    if(songNumLoad >= 0)
        fprintf(stdout, " - Attempt to load song number: %d / %d\n", songNumLoad, songsCount);
    else if(songsCount > 0)
        fprintf(stdout, " - File contains %d song(s)\n", songsCount);

    fprintf(stdout, "\n==========================================\n");
    flushout(stdout);

    fprintf(stdout, "                                               \r");

    s_is_playing = SDL_TRUE;
    /* Start playing */
    SDL_PauseAudio(0);

    /* wait until we're don't playing */
    while(s_is_playing)
    {
        SDL_LockAudio();
        time_pos = edmidi_positionTell(midi_player);
        SDL_UnlockAudio();
        milliseconds = (uint64_t)(time_pos * 1000.0);

        if(milliseconds != milliseconds_prev)
        {
            if(printsCounter >= printsCounterPeriod)
            {
                printsCounter = -1;
                secondsToHMSM(time_pos, posHMS, 25);
                fprintf(stdout, "                                               \r");
                fprintf(stdout, "Time position: %s / %s\r", posHMS, totalHMS);
                flushout(stdout);
                milliseconds_prev = milliseconds;
            }
            printsCounter++;
        }
        SDL_Delay(100);
    }

    fprintf(stdout, "                                               \n\n");
    flushout(stdout);

    /* shut everything down */
    SDL_CloseAudio();
    edmidi_close(midi_player);

    return 0 ;
}


/*
 audio callback function
 here you have to copy the data of your audio buffer into the
 requesting audio buffer (stream)
 you should only copy as much as the requested length (len)
*/
static void my_audio_callback(void *midi_player, Uint8 *stream, int len)
{
    struct EDMIDIPlayer *mp = *(struct EDMIDIPlayer**)midi_player;
    int count = len / s_audioFormat.containerSize;

    /* Take some samples from the EDMIDI */
    int got = edmidi_playFormat(mp, count,
                                stream,
                                stream + s_audioFormat.containerSize,
                                &s_audioFormat);

    if(got == 0)
    {
        s_is_playing = SDL_FALSE;
        SDL_memset(stream, 0, len);
    }
}
