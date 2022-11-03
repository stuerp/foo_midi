comment ~
---------------------------------------------------------------------
	BASS 3D test
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
  include \masm32\include\gdi32.inc
  include \masm32\include\user32.inc
  include \masm32\include\kernel32.inc
  include \masm32\include\msvcrt.inc
  include \masm32\include\Comctl32.inc
  include \masm32\include\comdlg32.inc
  include \masm32\include\oleaut32.inc
  include \masm32\macros\macros.asm		; masm32 macro file

  include ..\bass.inc

; libraries
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  includelib \masm32\lib\masm32.lib
  includelib \masm32\lib\gdi32.lib
  includelib \masm32\lib\user32.lib
  includelib \masm32\lib\kernel32.lib
  includelib \masm32\lib\msvcrt.lib
  includelib \masm32\lib\Comctl32.lib
  includelib \masm32\lib\comdlg32.lib
  includelib \masm32\lib\oleaut32.lib

  includelib ..\..\c\bass.lib

  ; channel (sample/music) info structure
  TChannel STRUCT
    Channel		DWORD	?		; the channel
    pos			BASS_3DVECTOR <>	; position
    vel			BASS_3DVECTOR <>	; velocity
  TChannel ENDS

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

  ; Messaging macros
  ITEM MACRO  args : VARARG
    arg equ <invoke GetDlgItem>			;; construct invoke and function name
    FOR var, <args>				;; loop through all arguments
      arg CATSTR arg, <, reparg(var)>   	;; replace quotes and append arg
    ENDM
    arg						;; write the invoke macro
    EXITM <eax>
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

  ZeroMemory		EQU	<RtlZeroMemory>
  MoveMemory		EQU	<RtlMoveMemory>
  TIMERPERIOD		EQU	50		; timer period (ms)
  MAXDIST		EQU	50		; maximum distance of the channels (m)
  SPEED			EQU	12		; speed of the channels' movement (m/s)

  ; Declared procedures
  WinMain		PROTO
  WndProc		PROTO :DWORD,:DWORD,:DWORD,:DWORD

.data
  Chans			DWORD	0		; the channels
  ChanCnt		DWORD	0		; number of channels
  ChanCur		DWORD	-1		; current channel

  szError		BYTE	'Error', 0
  szNotDevice		BYTE	'Can''t initialize output device', 0
  szNotDialog		BYTE	'Can''t create window', 0
  szIncorrectVersion	BYTE	'An incorrect version of BASS.DLL was loaded', 0
  szNotLoadFile		BYTE	'Can''t load file (note samples must be mono)', 0
  szFiles		BYTE	'playable files', 0, '*.wav;*.aif;*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx', 0, \
                                'All files', 0, '*.*', 0, 0

  EffectArray		BYTE	'Off', 0,         'Generic', 0,    'Padded Cell', 0
			BYTE	'Room', 0,        'Bathroom', 0,   'Living Room', 0
			BYTE	'Stone Room', 0,  'Auditorium', 0, 'Concert Hall', 0
			BYTE	'Cave', 0,        'Arena', 0,      'Hangar', 0
			BYTE	'Carpeted Hallway', 0, 'Hallway', 0, 'Stone Corridor', 0
			BYTE	'Alley', 0,       'Forest', 0,     'City', 0
			BYTE	'Mountains', 0,   'Quarry', 0,     'Plain', 0
			BYTE	'Parking Lot', 0, 'Sewer Pipe', 0, 'Under Water', 0
			BYTE	'Drugged', 0,     'Dizzy', 0,      'Psychotic', 0

.data?
  hInstance		DWORD	?
  hCursor		DWORD	?
  Wnd			DWORD	?

  szFileName		BYTE	MAX_PATH dup(?)
  szEffect		DWORD	27	dup(?)
  ofn			OPENFILENAME <>		; structure
  icex			INITCOMMONCONTROLSEX <>	; structure for Controls

  CHANNEL MACRO
    mov   eax, sizeof TChannel
    imul  ChanCur
    add   eax, Chans
  ENDM

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
  LOCAL temp[200] : BYTE

  invoke crt_sprintf, ADDR temp, SADD('%s', 13, 10, '(error code: %d)', 13, 10), Value1, rv(BASS_ErrorGetCode)
  invoke MessageBox, Wnd, ADDR temp, Value2, MB_OK or MB_ICONERROR
  ret

Error endp

;-----------------------------------------------------------

Update proc aWnd, uMsg, idEvent, dwTime : DWORD
  LOCAL DrawDC     : HDC
  LOCAL aRect      : RECT
  LOCAL temp, chnl : DWORD
  LOCAL red, green : HBRUSH

  mov    red,    rv(CreateSolidBrush, 0FFH)
  mov    green,  rv(CreateSolidBrush, 0FF00H)
  mov    aWnd,   ITEM(Wnd, 30)
  mov    DrawDC, rv(GetDC, aWnd)
  invoke GetClientRect, aWnd, ADDR aRect
  mov    esi,    aRect.right			; cx
  shr    esi,    1
  mov    edi,    aRect.bottom			; cy
  shr    edi,    1

  ; clear the display
  invoke FillRect, DrawDC, ADDR aRect, rv(GetStockObject, WHITE_BRUSH)

  ; Draw the listener
  invoke SelectObject, DrawDC, rv(GetStockObject, GRAY_BRUSH)
  lea    eax, [esi - 4]
  lea    ebx, [esi + 4]
  lea    ecx, [edi - 4]
  lea    edx, [edi + 4]
  invoke Ellipse, DrawDC, eax, ecx, ebx, edx

  xor ebx, ebx
  .while ebx < ChanCnt
    ; If the channel's playing then update it's position
    mov   eax, sizeof TChannel
    imul  ebx
    add   eax, Chans
    mov   chnl, eax

    .if rv(BASS_ChannelIsActive, [eax.TChannel].Channel) == BASS_ACTIVE_PLAYING
      mov   edx, chnl

      ; Check if channel has reached the max distance
      fld   [edx.TChannel].pos.z
      fistp temp
      .if SDWORD PTR temp >= MAXDIST || SDWORD PTR temp <= -MAXDIST
        fld  [edx.TChannel].vel.z
        fchs
        fstp [edx.TChannel].vel.z;
      .endif

      fld   [edx.TChannel].pos.x
      fistp temp
      .if SDWORD PTR temp >= MAXDIST || SDWORD PTR temp <= -MAXDIST
        fld  [edx.TChannel].vel.x
        fchs
        fstp [edx.TChannel].vel.x
      .endif

      ; Update channel position
      fld  [edx.TChannel].vel.z
      fmul FP8(50.0)				; TIMERPERIOD
      fdiv FP8(1000.0)
      fadd [edx.TChannel].pos.z
      fstp [edx.TChannel].pos.z

      fld  [edx.TChannel].vel.x
      fmul FP8(50.0)				; TIMERPERIOD
      fdiv FP8(1000.0)
      fadd [edx.TChannel].pos.x
      fstp [edx.TChannel].pos.x

      invoke BASS_ChannelSet3DPosition, [edx.TChannel].Channel, ADDR [edx.TChannel].pos, NULL, ADDR [edx.TChannel].vel
    .endif

    .if ChanCur == ebx
      mov eax, green
    .else
      mov eax, red
    .endif
    invoke SelectObject, DrawDC, eax

    mov   edx, chnl
    ; Draw the channel position indicator
    lea   eax, [esi - 10]
    mov   temp, eax
    fld   [edx.TChannel].pos.x
    fimul temp
    fdiv  FP8(50.0)				; MAXDIST
    fistp temp
    mov   ecx, esi
    add   ecx, temp				; x

    lea   eax, [edi - 10]
    mov   temp, eax
    fld   [edx.TChannel].pos.z
    fimul temp
    fdiv  FP8(50.0)				; MAXDIST
    fistp temp
    mov   edx, edi
    sub   edx, temp				; y

    push   ebx
    lea    eax, [ecx - 4]			; x - 4
    lea    ebx, [ecx + 4]			; x + 4
    lea    ecx, [edx - 4]			; y - 4
    lea    edx, [edx + 4]			; y + 4
    invoke Ellipse, DrawDC, eax, ecx, ebx, edx
    pop    ebx

    inc ebx
  .endw

  ; Apply the 3D changes
  invoke BASS_Apply3D

  invoke ReleaseDC, aWnd, DrawDC
  invoke DeleteObject, red
  invoke DeleteObject, green

  ret

Update endp

;-----------------------------------------------------------
; Update the button states

UpdateButtons proc
  LOCAL temp : DWORD

  mov ebx, 12
  .repeat
    .if ChanCur == -1
      mov esi, FALSE
    .else
      mov esi, TRUE
    .endif

    invoke EnableWindow, ITEM(Wnd, ebx), esi

    inc ebx
  .until ebx > 19

  .if ChanCur != -1
    CHANNEL
    mov   esi, eax

    fld   [esi.TChannel].vel.x
    fabs
    fistp temp
    mov   ebx, [temp]
    invoke SetDlgItemInt, Wnd, 15, ebx, FALSE

    fld   [esi.TChannel].vel.z
    fabs
    fistp temp
    mov   ebx, [temp]
    invoke SetDlgItemInt, Wnd, 17, ebx, FALSE
  .endif

  ret

UpdateButtons endp

;-----------------------------------------------------------

LoadFile proc
  LOCAL p64 : QWORD				; for 64-bit parameters

  invoke ZeroMemory, ADDR ofn, sizeof OPENFILENAME
  mov    szFileName[0],   0			; set 1st byte to zero
  mov    ofn.lStructSize, sizeof OPENFILENAME
  m2m    ofn.hWndOwner,   Wnd
  m2m    ofn.hInstance,   hInstance
  mov    ofn.nMaxFile,    MAX_PATH
  mov    ofn.Flags,       OFN_FILEMUSTEXIST or OFN_HIDEREADONLY or OFN_EXPLORER
  m2m    ofn.lpstrFilter, offset szFiles             
  m2m    ofn.lpstrFile,   offset szFileName

  xor esi, esi
  .if rv(GetOpenFileName, ADDR ofn) && szFileName[0]
    ; Load a music or sample from "file"
    mov DWORD PTR [p64], 0
    mov DWORD PTR [p64 + 4], 0
    mov esi, rv(BASS_MusicLoad, FALSE, ADDR szFileName, p64, 0, BASS_MUSIC_RAMP + BASS_SAMPLE_LOOP + BASS_SAMPLE_3D, 1)
    .if !eax
      mov DWORD PTR [p64], 0
      mov DWORD PTR [p64 + 4], 0
      mov esi, rv(BASS_SampleLoad, FALSE, ADDR szFileName, p64, 0, 1, BASS_SAMPLE_LOOP + BASS_SAMPLE_3D + BASS_SAMPLE_MONO)
      .if !eax
        return FALSE
      .endif
    .endif
  .else
    return FALSE
  .endif

  return TRUE

LoadFile endp

;-----------------------------------------------------------
; calculates address for literal strings
;-----------------------------------------------------------

initurlname proc

  ; init Urls table address
  lea esi, EffectArray
  xor ebx, ebx
  .repeat
    mov szEffect[ebx * sizeof DWORD], esi
    .repeat
      lodsb
    .until al == 0
    inc ebx
  .until ebx == 27
  
  ret

initurlname endp

;-----------------------------------------------------------

InitApp proc

  invoke initurlname
  xor    ebx, ebx
  .repeat
    CTRL Wnd, 22, CB_ADDSTRING, 0, szEffect[ebx * sizeof DWORD]
    inc ebx
  .until ebx == 27

  CTRL Wnd, 22, CB_SETCURSEL, 0, 0

  MESS Wnd, 16, UDM_SETRANGE, 0,     MAKELONG(99, 0)
  MESS Wnd, 18, UDM_SETRANGE, 0,     MAKELONG(99, 0)
  MESS Wnd, 20, TBM_SETRANGE, FALSE, MAKELONG(0, 20)
  MESS Wnd, 20, TBM_SETPOS,   TRUE,  10
  MESS Wnd, 21, TBM_SETRANGE, FALSE, MAKELONG(0, 20)
  MESS Wnd, 21, TBM_SETPOS,   TRUE,  10

  invoke SetTimer, Wnd, 1, TIMERPERIOD, Update

  return TRUE

InitApp endp

;-----------------------------------------------------------

DialogProc proc Win, uMsg, wParam, lParam : DWORD
  LOCAL pos : SDWORD
  LOCAL p64 : QWORD				; for 64-bit parameters

  Switch uMsg
    case WM_INITDIALOG
      mrm    Wnd, Win
      invoke InitApp
      .if eax == FALSE
        invoke SendMessage, Win, WM_CLOSE, 0, 0
        return FALSE
      .endif
      return TRUE

    case WM_HSCROLL
      .if lParam
        invoke SendMessage, lParam, TBM_GETPOS, 0, 0
        lea    eax, [eax - 10]
        mov    pos, eax
        fild   pos
        fdiv   FP4(5.0)
        fstp   p64
        invoke crt_pow, FP8(2.0), p64
        fstp   pos

        Switch rv(GetDlgCtrlID, lParam)
          case 20				; change the rolloff factor
            invoke BASS_Set3DFactors, FP4(-1.0), pos, FP4(-1.0)
          case 21				; change the doppler factor
            invoke BASS_Set3DFactors, FP4(-1.0), FP4(-1.0), pos
        endsw
      .endif

    case WM_COMMAND
      LOWORD wParam
      Switch eax
        case 10					; change the selected channel
          HIWORD wParam
          .if eax == LBN_SELCHANGE
            CTRL Win, 10, LB_GETCURSEL, 0, 0
            mov ChanCur, eax
            .if ChanCur == LB_ERR
              mov ChanCur, -1
            .endif
            invoke UpdateButtons
          .endif

        case 11					; add a channel
          .if rv(LoadFile)			; return: esi -> channel
            inc    ChanCnt
            mov    eax, sizeof TChannel
            imul   ChanCnt
            mov    ebx, eax
            mov    Chans, rv(crt_realloc, Chans, eax)
            lea    eax, [eax + ebx]
            sub    eax, sizeof TChannel
            mov    ebx, eax
            invoke ZeroMemory, eax, sizeof TChannel	; ZeroMemory smaller than crt_memset
            mov    DWORD PTR [ebx.TChannel].Channel, esi
            invoke BASS_SampleGetChannel, esi, FALSE	; initialize sample channel
            invoke crt_strrchr, ADDR szFileName, '\\'
            inc    eax
            CTRL   Win, 10, LB_ADDSTRING, 0, eax
          .elseif !esi
            invoke Error, ADDR szNotLoadFile, ADDR szError
          .endif

        case 12					; remove a channel
          CHANNEL
          mov    esi, eax
          invoke BASS_SampleFree, [esi.TChannel].Channel
          invoke BASS_MusicFree, [esi.TChannel].Channel
          lea    ebx, [esi + sizeof TChannel]
          mov    eax, ChanCnt
          sub    eax, ChanCur
          dec    eax
          imul   eax, sizeof TChannel
          invoke crt_memmove, esi, ebx, eax
          
          dec    ChanCnt
          CTRL   Win, 10, LB_DELETESTRING, ChanCur, 0
          mov    ChanCur, -1
          invoke UpdateButtons

        case 13
          CHANNEL
          mov    eax, DWORD PTR [eax.TChannel].Channel
          invoke BASS_ChannelPlay, eax, FALSE

        case 14
          CHANNEL
          invoke BASS_ChannelPause, DWORD PTR [eax.TChannel].Channel

        case 15					; X velocity
          .if ChanCur != -1
            HIWORD wParam
            .if eax == EN_CHANGE
              mov   ebx, rv(GetDlgItemInt, Win, 15, 0, FALSE)
              CHANNEL
              fld   [eax.TChannel].vel.x
              fabs
              fistp pos
              .if ebx != pos
                mov  pos, ebx
                fild pos
                fstp [eax.TChannel].vel.x
              .endif
            .endif
          .endif

        case 17					; Z velocity
          .if ChanCur != -1
            HIWORD wParam
            .if eax == EN_CHANGE
              mov   ebx, rv(GetDlgItemInt, Win, 17, 0, FALSE)
              CHANNEL
              fld   [eax.TChannel].vel.z
              fabs
              fistp pos
              .if ebx != pos
                mov  pos, ebx
                fild pos
                fstp [eax.TChannel].vel.z
              .endif
            .endif
          .endif

        case 19					; reset the position and velocity to 0
          CHANNEL
          mov    ebx, eax
          ; ZeroMemory smaller than crt_memset
          invoke ZeroMemory, ADDR [ebx.TChannel].pos, sizeof TChannel.pos
          invoke ZeroMemory, ADDR [ebx.TChannel].vel, sizeof TChannel.vel
          invoke UpdateButtons

        case 22					; change the EAX environment
          HIWORD wParam
          .if eax == CBN_SELCHANGE
            CTRL Win, 22, CB_GETCURSEL, 0, 0
            .if !eax
              ; off (volume=0)
              invoke BASS_SetEAXParameters, -1, FP4(0.0), FP4(-1.0), FP4(-1.0)
            .else
              dec    eax
              invoke BASS_SetEAXParameters, eax, FP4(-1.0), FP4(-1.0), FP4(-1.0)
            .endif
          .endif
      endsw

    case WM_CLOSE
      invoke KillTimer, Win, 1
      .if Chans
        invoke crt_free, Chans
      .endif
      invoke BASS_Free
      invoke PostQuitMessage, 0
  endsw

  return FALSE

DialogProc endp

;-----------------------------------------------------------

WinMain proc
  LOCAL win    : DWORD
  LOCAL msg    : MSG
  LOCAL device : DWORD

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
  mov win, rv(CreateDialogParam, hInstance, 1000, 0, ADDR DialogProc, 0)
  .if !eax
    invoke Error, ADDR szNotDialog, ADDR szError
    return FALSE
  .endif

  ; Initialize the default output device with 3D support
  .if !rv(BASS_Init, -1, 44100, BASS_DEVICE_3D, win, NULL)
    invoke Error, ADDR szNotDevice, ADDR szError
    invoke EndDialog, win, 0
    return 0
  .endif

  ; Use meters as distance unit, real world rolloff, real doppler effect
  invoke BASS_Set3DFactors, FP4(1.0), FP4(1.0), FP4(1.0)
  ; Turn EAX off (volume=0), if error then EAX is not supported
  .if rv(BASS_SetEAXParameters, -1, FP4(0.0), FP4(-1.0), FP4(-1.0))
    invoke EnableWindow, ITEM(win, 22), TRUE
  .endif

  ;===================================
  ; Loop until PostQuitMessage is sent
  ;===================================
  .while TRUE
    invoke GetMessage, ADDR msg, NULL, 0, 0
    .break .if (!eax)
    .if !rv(IsDialogMessage, win, ADDR msg)
      invoke TranslateMessage, ADDR msg
      invoke DispatchMessage,  ADDR msg
    .endif
  .endw

  invoke BASS_Free

  return FALSE

WinMain endp

;-----------------------------------------------------------

end start
