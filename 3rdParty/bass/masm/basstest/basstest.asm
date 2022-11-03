comment ~
---------------------------------------------------------------------
	BASS simple playback test
	Copyright (c) 1999-2008 Un4seen Developments Ltd.

        C++ to MASM adapted by Evgeny Melnikov

        http://dsoft1961.narod.ru
        mail angvelem@gmail.com
---------------------------------------------------------------------~

  .586                      ; create 32 bit code
  .model flat, stdcall      ; 32 bit memory model
  option casemap :none      ; case sensitive

; include files
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  include \masm32\include\windows.inc
  include \masm32\include\masm32.inc
  include \masm32\include\user32.inc
  include \masm32\include\kernel32.inc
  include \masm32\include\msvcrt.inc
  include \masm32\include\Comctl32.inc
  include \masm32\include\comdlg32.inc
  include \masm32\macros\macros.asm		; masm32 macro file

  include ..\bass.inc


; libraries
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  includelib \masm32\lib\masm32.lib
  includelib \masm32\lib\user32.lib
  includelib \masm32\lib\kernel32.lib
  includelib \masm32\lib\msvcrt.lib
  includelib \masm32\lib\Comctl32.lib
  includelib \masm32\lib\comdlg32.lib

  includelib ..\..\c\bass.lib

  ;=============
  ; Local macros
  ;=============

  exit MACRO
    invoke ExitProcess, 0
  ENDM

  HIWORD MACRO bigword				; Retrieves the high word from double word 
    mov	eax, bigword
    shr	eax, 16					; Shift 16 for high word to set to high word
  ENDM

  MESS MACRO args : VARARG
    arg equ <invoke SendDlgItemMessage>		;; construct invoke and function name
    FOR var, <args>				;; loop through all arguments
      arg CATSTR arg, <, reparg(var)>   	;; replace quotes and append arg
    ENDM
    arg						;; write the invoke macro
  ENDM

  CTRL MACRO win, m, id, w, l
    MESS win, m, id, w, l
  ENDM

  MAKELONG MACRO A, B : REQ
    mov eax, B
    shl eax, 16
    or  eax, A
    EXITM <eax>
  ENDM

  GETHANDLE MACRO Str
    imul   eax, sizeof Str
    add    eax, [Str]
    mov    eax, [eax]
    EXITM <eax>
  ENDM

  HMUSIC		TYPEDEF	DWORD		; MOD music handle
  HSAMPLE		TYPEDEF	DWORD		; sample handle
  HSTREAM		TYPEDEF	DWORD		; sample stream handle

  ZeroMemory		EQU	<RtlZeroMemory>
  MoveMemory		EQU	<RtlMoveMemory>

  ; Declared procedures
  WinMain		PROTO

.data
  strs			HSTREAM	0
  mods			HMUSIC	0
  sams			HSAMPLE	0
  strc			DWORD	0
  modc			DWORD	0
  samc			DWORD	0

  szError		BYTE	'Error', 0
  szNotDevice		BYTE	'Can''t initialize device', 0
  szIncorrectVersion	BYTE	'An incorrect version of BASS.DLL was loaded', 0
  szNotOpen		BYTE	'Can''t open', 0
  szNotPlay		BYTE	'Can''t play', 0
  szStream		BYTE	'stream', 0
  szMusic		BYTE	'music', 0
  szSample		BYTE	'sample', 0

  szStreamFilter	BYTE	'Streamable files', 0, '*.wav;*.aif;*.mp3;*.mp2;*.mp1;*.ogg', 0,     'All files', 0, '*.*', 0, 0
  szMusicFilter		BYTE	'MOD music files', 0,  '*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx', 0, 'All files', 0, '*.*', 0, 0
  szSampleFilter	BYTE	'Sample files', 0,     '*.wav;*.aif', 0,                             'All files', 0, '*.*', 0, 0

.data?
  hInstance		DWORD	?
  hCursor		DWORD	?
  Wnd			DWORD	?
  SysVolume		DWORD	?

  szFileName		BYTE	MAX_PATH dup(?)
  ofn			OPENFILENAME <>		; structure
  icex			INITCOMMONCONTROLSEX <>	; structure for Controls

.code

start:

  invoke InitCommonControls

  ; enable trackbar support
  mov    icex.dwSize, sizeof icex
  mov    icex.dwICC, ICC_BAR_CLASSES
  invoke InitCommonControlsEx, ADDR icex

  mov    hInstance, rv(GetModuleHandle, NULL)
  mov    hCursor,   rv(LoadCursor, NULL, IDC_ARROW)

  invoke WinMain
  exit

;-----------------------------------------------------------

Error proc Value1, Value2 : DWORD
  LOCAL temp[200]      : BYTE

  invoke crt_sprintf, ADDR temp, SADD('%s', 13, 10, '(error code: %d)', 13, 10), Value1, rv(BASS_ErrorGetCode)
  invoke MessageBox, Wnd, ADDR temp, Value2, MB_OK or MB_ICONERROR
  ret

Error endp

;-----------------------------------------------------------

ReallocMem proc Chans, Chanc, CtrlID : DWORD

  mov    ebx, eax
  mov    esi, Chanc
  inc    DWORD PTR [esi]
  mov    eax, sizeof Chans
  imul   DWORD PTR [esi]
  lea    esi, [eax - sizeof Chans]
  mov    edi, Chans
  mov    DWORD PTR [edi], rv(crt_realloc, DWORD PTR [edi], eax)
  lea    esi, [eax + esi]
  mov    DWORD PTR [esi], ebx
  invoke crt_strrchr, ADDR szFileName, '\\'
  inc    eax
  CTRL   Wnd, CtrlID, LB_ADDSTRING, 0, eax

  ret

ReallocMem endp

;-----------------------------------------------------------

MoveMem proc Chans, Chanc, CtrlID : DWORD

  CTRL   Wnd, CtrlID, LB_DELETESTRING, ebx, 0
  mov    esi, Chanc
  dec    DWORD PTR [esi]
  mov    eax, Chans
  mov    eax, [eax]
  lea    edi, [eax + ebx * sizeof Chans]	; dest
  lea    esi, [edi + sizeof Chans]		; src
  mov    eax, Chanc
  mov    eax, [eax]
  sub    eax, ebx
  imul   eax, sizeof Chans			; count
  invoke MoveMemory, edi, esi, eax

  ret

MoveMem endp

;-----------------------------------------------------------

LoadFile proc aFilter : DWORD

  invoke ZeroMemory, ADDR ofn, sizeof OPENFILENAME
  mov    szFileName[0],   0			; set 1st byte to zero
  mov    ofn.lStructSize, sizeof OPENFILENAME
  m2m    ofn.hWndOwner,   Wnd
  m2m    ofn.hInstance,   hInstance
  mov    ofn.nMaxFile,    MAX_PATH
  mov    ofn.Flags,       OFN_FILEMUSTEXIST or OFN_HIDEREADONLY or OFN_EXPLORER
  m2m    ofn.lpstrFilter, aFilter
  m2m    ofn.lpstrFile,   offset szFileName

  .if !rv(GetOpenFileName, ADDR ofn) || !szFileName[0]
    return FALSE
  .endif

  return TRUE

LoadFile endp

;-----------------------------------------------------------

InitApp proc
  LOCAL temp : DWORD

  ; setup output device
  .if !rv(BASS_Init, -1, 44100, 0, Wnd, NULL)
    invoke Error, ADDR szNotDevice, szError
    return FALSE
  .endif

  ; initialize volume sliders
  MESS Wnd, 16, TBM_SETRANGE, 1, MAKELONG(0, 100)
  MESS Wnd, 16, TBM_SETPOS,   1, 100
  MESS Wnd, 26, TBM_SETRANGE, 1, MAKELONG(0, 100)
  MESS Wnd, 26, TBM_SETPOS,   1, 100
  MESS Wnd, 34, TBM_SETRANGE, 1, MAKELONG(0, 100)
  MESS Wnd, 34, TBM_SETPOS,   1, 100
  MESS Wnd, 43, TBM_SETRANGE, 1, MAKELONG(0, 100)

  invoke BASS_GetVolume
  fst    SysVolume
  fmul   FP8(100.0)
  fistp  temp
  MESS   Wnd, 43, TBM_SETPOS,   1, temp
  invoke SetTimer, Wnd, 1, 250, NULL

  return TRUE

InitApp endp

;-----------------------------------------------------------

DialogProc proc Win, uMsg, wParam, lParam : DWORD
  LOCAL Buf[200] : BYTE
  LOCAL temp    : DWORD
  LOCAL p64     : QWORD				; for 64-bit parameters

  Switch uMsg
    case WM_INITDIALOG
      mrm    Wnd, Win
      invoke InitApp
      .if eax == FALSE
        invoke SendMessage, Win, WM_CLOSE, 0, 0
        return FALSE
      .endif
      return TRUE

    case WM_TIMER
      ; update the CPU usage % display
      invoke BASS_GetCPU
      fstp   p64
      invoke crt_sprintf, ADDR Buf, SADD('%.2f'), p64
      MESS Win, 40, WM_SETTEXT, 0, ADDR Buf

    case WM_COMMAND
      LOWORD wParam
      Switch eax
        case IDCANCEL
          invoke SendMessage, Win, WM_CLOSE, 0, 0
    	  return TRUE

    	; Section of Stream
        case 11
          CTRL Win, 10, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; play the stream (continue from current position)
            invoke BASS_ChannelPlay, GETHANDLE(strs), FALSE
            .if !eax
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotPlay, ADDR szStream
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 12
          CTRL Win, 10, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; stop the stream
            invoke BASS_ChannelStop, GETHANDLE(strs)
          .endif

        case 13
          CTRL Win, 10, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; play the stream from the start
            invoke BASS_ChannelPlay, GETHANDLE(strs), TRUE
            .if !eax
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotPlay, ADDR szStream
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 14
          .if rv(LoadFile, ADDR szStreamFilter)
            mov DWORD PTR [p64], 0
            mov DWORD PTR [p64 + 4], 0
            .if rv(BASS_StreamCreateFile, FALSE, ADDR szFileName, p64, p64, 0)
              invoke ReallocMem, ADDR strs, ADDR strc, 10
            .else
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotOpen, ADDR szStream
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 15
          CTRL Win, 10, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            mov    ebx, eax
            invoke BASS_StreamFree, GETHANDLE(strs)	; free the stream
            invoke MoveMem, ADDR strs, ADDR strc, 10
          .endif

        ; Section of Music
        case 21
          CTRL Win, 20, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; play the music (continue from current position)
            invoke BASS_ChannelPlay, GETHANDLE(mods), FALSE
            .if !eax
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotPlay, ADDR szMusic
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 22
          CTRL Win, 20, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; stop the music
            invoke BASS_ChannelStop, GETHANDLE(mods)
          .endif

        case 23
          CTRL Win, 20, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; play the music from the start
            invoke BASS_ChannelPlay, GETHANDLE(mods), TRUE
            .if !eax
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotPlay, ADDR szMusic
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 24
          .if rv(LoadFile, ADDR szMusicFilter)
            mov DWORD PTR [p64], 0
            mov DWORD PTR [p64 + 4], 0
            ; load a music from "file" with ramping enabled
            .if rv(BASS_MusicLoad, FALSE, ADDR szFileName, p64, 0, BASS_MUSIC_RAMPS, 1)
              invoke ReallocMem, ADDR mods, ADDR modc, 20
            .else
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotOpen, ADDR szMusic
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 25
          CTRL Win, 20, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            mov    ebx, eax
            invoke BASS_MusicFree, GETHANDLE(mods)	; free the music
            invoke MoveMem, ADDR mods, ADDR modc, 20
          .endif

        ; Section of Sample
        case 31
          CTRL Win, 30, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            ; play the sample (at default rate, volume=50%, random pan position)
            mov    ebx, rv(BASS_SampleGetChannel, GETHANDLE(sams), FALSE)
            invoke BASS_ChannelSetAttribute, ebx, BASS_ATTRIB_VOL, FP4(0.5)
            invoke nrandom, 201
            sub    eax, 100
            mov    temp, eax
            fild   temp
            fdiv   FP4(100.0)
            fstp   temp
            invoke BASS_ChannelSetAttribute, ebx, BASS_ATTRIB_PAN, temp
            invoke BASS_ChannelPlay, ebx, FALSE
            .if !eax
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotPlay, ADDR szSample
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 32
          .if rv(LoadFile, ADDR szSampleFilter)
            mov DWORD PTR [p64], 0
            mov DWORD PTR [p64 + 4], 0
            ; Load a sample from "file" and give it a max of 3 simultaneous
            ; playings using playback position as override decider
            .if rv(BASS_SampleLoad, FALSE, ADDR szFileName, p64, 0, 3, BASS_SAMPLE_OVER_POS)
              invoke ReallocMem, ADDR sams, ADDR samc, 30
            .else
              invoke crt_sprintf, ADDR Buf, SADD('%s %s'), ADDR szNotOpen, ADDR szSample
              invoke Error, ADDR Buf, ADDR szError
            .endif
          .endif

        case 33
          CTRL Win, 30, LB_GETCURSEL, 0, 0
          .if eax != LB_ERR
            mov    ebx, eax
            invoke BASS_SampleFree, GETHANDLE(sams)	; free the sample
            invoke MoveMem, ADDR sams, ADDR samc, 30
          .endif

        case 41
          invoke BASS_Pause			; pause output

        case 42
          invoke BASS_Start			; resume output

        case 44
          CTRL Win, 44, BM_GETCHECK, 0, 0
          inc eax				; BST_UNCHECKED = 0, BST_CHECKED = 1
          ; set 1 or 2 update threads
          invoke BASS_SetConfig, BASS_CONFIG_UPDATETHREADS, eax
      endsw
    
    case WM_HSCROLL
      LOWORD wParam
      .if lParam && eax != SB_THUMBPOSITION && eax != SB_ENDSCROLL
      	invoke SendMessage, lParam, TBM_GETPOS, 0, 0
      	mov    temp, eax
      	imul   eax, 100
      	mov    ebx, eax
      	fild   temp
      	fdiv   FP4(100.0)
      	fstp   temp
        Switch rv(GetDlgCtrlID, lParam)
          case 16
            ; global stream volume (0-10000)
            invoke BASS_SetConfig, BASS_CONFIG_GVOL_STREAM, ebx

          case 26
            ; global MOD volume (0-10000)
            invoke BASS_SetConfig, BASS_CONFIG_GVOL_MUSIC, ebx

          case 34
            ; global sample volume (0-10000)
            invoke BASS_SetConfig, BASS_CONFIG_GVOL_SAMPLE, ebx

          case 43
            ; output volume (0-1)
            invoke BASS_SetVolume, temp
        endsw
      .endif
    
    case WM_CLOSE
      invoke KillTimer, Win, 1
      invoke BASS_SetVolume, SysVolume		; Restore volume
      invoke BASS_Free				; close output
      invoke EndDialog, Win, 0
  endsw

  return FALSE

DialogProc endp

;-----------------------------------------------------------

WinMain proc

  ; check the correct BASS was loaded
  invoke BASS_GetVersion
  HIWORD eax
  .if ax != BASSVERSION
    invoke Error, ADDR szIncorrectVersion, ADDR szError
    return FALSE
  .endif

  ; -------------------------------------------
  ; Call the dialog box stored in resource file
  ;
  ; Display the window
  ; -------------------------------------------
  invoke DialogBoxParam, hInstance, 1000, 0, ADDR DialogProc, 0

  return FALSE

WinMain endp

;-----------------------------------------------------------

end start
