#include <stdio.h>
#include <SDL2/SDL.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include "CSMFPlay.hpp"

/* variable declarations */
static Uint32 is_playing = 0; /* remaining length of the sample we have to play */

struct MidPlayInstance
{
    dsa::CSMFPlay *p;
};

static void stop_playing(int)
{
    is_playing = 0;
}

/*
 audio callback function
 here you have to copy the data of your audio buffer into the
 requesting audio buffer (stream)
 you should only copy as much as the requested length (len)
*/
void my_audio_callback(void *midi_player, Uint8 *stream, int len)
{
    MidPlayInstance *mp = (MidPlayInstance*)midi_player;
    int count = len / sizeof(float);
    float* dest = (float*)stream;
    DWORD got = mp->p->RenderF32(dest, (count >> 1));
    if(got == 0)
        is_playing = 0;
}

//static volatile int done = 0 ;

int main(int argc, char *argv[])
{
    static SDL_AudioSpec            spec; /* the specs of our piece of music */
    MidPlayInstance mp;
    mp.p = new dsa::CSMFPlay(44100, 8);

    signal(SIGINT, &stop_playing);
    signal(SIGTERM, &stop_playing);

    if(argc<=1) {
        printf("%s filename.mid\n", argv[0]);
        return 0;
    }

    if(!mp.p->Open(argv[1])) {
        printf("File open error!\n");
        return 1;
    }

    mp.p->SetLoop(true);

    mp.p->Start();


    /* Initialize SDL.*/
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
        return 1;

    memset(&spec, 0, sizeof(SDL_AudioSpec));
    spec.freq = 44100;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 4096;

    /* set the callback function */
    spec.callback = my_audio_callback;
    /* set ADLMIDI's descriptor as userdata to use it for sound generation */
    spec.userdata = &mp;

    /* Open the audio device */
    if (SDL_OpenAudio(&spec, NULL) < 0)
    {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        return 1;
    }

    is_playing = 1;
    /* Start playing */
    SDL_PauseAudio(0);

    printf("Playing... Hit Ctrl+C to quit!\n");

    /* wait until we're don't playing */
    while(is_playing)
    {
        SDL_Delay(100);
    }

    /* shut everything down */
    SDL_CloseAudio();

    delete mp.p;

    return 0 ;
}
