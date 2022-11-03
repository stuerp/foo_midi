#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include "CSMFPlay.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

static const nch=2;
static const sample_rate=96000;
static short buf[2][sample_rate*nch] ;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
  switch(uMsg)
  {
  case WOM_OPEN:
    break ;
  case WOM_CLOSE:
    break ;
  case WOM_DONE:
    *((int *)dwInstance) = 1 ;
    break ;
  default:
    break ;
  }
  return ;
};

static volatile int done = 0 ;

int main(int argc, char *argv[])
{
#if defined(_MSC_VER)
#ifdef _DEBUG
  HANDLE hDbgfile;
  hDbgfile = CreateFile("D:\\emidi.log", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) ;
  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF ) ;
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE ) ;
  _CrtSetReportFile( _CRT_WARN, (HANDLE)hDbgfile ) ;
  //_CrtCrtSetBreakAlloc(0);
#endif
#endif

  HWAVEOUT hwo ;
  WAVEFORMATEX wfx ;
  WAVEHDR wh[2] ;
  int b  = 0 ,c , song = 0 ;
  dsa::CSMFPlay *smfplay = new dsa::CSMFPlay(sample_rate, 8);

  if(argc<=1) {
    printf("SMFPlay filename.mid\n");
    return 0;
  }

  if(!smfplay->Open(argv[1])) {
    printf("File open error!\n");
    return 1;
  }

  smfplay->Start();

  wfx.wFormatTag = WAVE_FORMAT_PCM ;
  wfx.nChannels = nch ;
  wfx.nSamplesPerSec = sample_rate ;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec ;
  wfx.wBitsPerSample = 16 ;
  wfx.nBlockAlign = wfx.nChannels ;
  wfx.cbSize = 0 ;

  if(waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD)waveOutProc, (DWORD)&done, CALLBACK_FUNCTION)!=MMSYSERR_NOERROR)
  {
    perror("Can't open a wave device.") ;
    exit(1) ;
  }

  smfplay->Render(buf[b], sample_rate);
  wh[b].lpData = (char *)buf[b] ;
  wh[b].dwBufferLength = sample_rate * nch * sizeof(short) ;
  wh[b].dwFlags = WHDR_DONE ;
  waveOutPrepareHeader(hwo, &wh[b], sizeof(WAVEHDR)) ;
  waveOutWrite(hwo, &wh[b], sizeof(WAVEHDR)) ;
  waveOutUnprepareHeader(hwo, &wh[b], sizeof(WAVEHDR)) ;

  bool pause = false;
  bool end_flag = false;

  while(!end_flag)
  {
    b = !b ;
    if(done) { waveOutPause(hwo); pause = true; }
    if(!smfplay->Render(buf[b], sample_rate)) {
      end_flag = true;
    } else {
      wh[b].lpData = (char *)buf[b] ;
      wh[b].dwBufferLength = sample_rate * nch * sizeof(short) ;
      wh[b].dwFlags = WHDR_DONE ;
      waveOutPrepareHeader(hwo, &wh[b], sizeof(WAVEHDR)) ;
      waveOutWrite(hwo, &wh[b], sizeof(WAVEHDR)) ;
      waveOutUnprepareHeader(hwo, &wh[b], sizeof(WAVEHDR)) ;
    }
    if(pause) { waveOutRestart(hwo); pause = false; }

    while(!done)
    {
      if(_kbhit()){
        c = _getch() ;
        switch(c)
        {
        case 'q' :
          goto quit ; 
        default:
          break;
        }
      }
    }
    done = 0 ;
  }

quit:

  waveOutClose(hwo) ;
  delete smfplay;

#if defined(_MSC_VER)
#if defined(_DEBUG)
  if(_CrtDumpMemoryLeaks()) MessageBox(NULL, "Memory is leaked.", "MEMORY LEAK", MB_OK) ;
  CloseHandle(hDbgfile) ;
#endif
#endif

  return 0 ;
}