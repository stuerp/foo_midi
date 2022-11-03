comment ~
----------------------------------------------------------------
	BASS simple synth
	Copyright (c) 2001-2008 Un4seen Developments Ltd.

        C++ to MASM adapted by Evgeny Melnikov

        http://dsoft1961.narod.ru
        mail angvelem@gmail.com
----------------------------------------------------------------~

  .586                      ; create 32 bit code
  .model flat, stdcall      ; 32 bit memory model
  option casemap :none      ; case sensitive

; include files
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  include \masm32\include\windows.inc
  include \masm32\include\masm32.inc
  include \masm32\include\kernel32.inc
  include \masm32\include\msvcrt.inc
  include \masm32\macros\macros.asm         ; masm32 macro file

  include ..\bass.inc


; libraries
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  includelib \masm32\lib\masm32.lib
  includelib \masm32\lib\kernel32.lib
  includelib \masm32\lib\msvcrt.lib

  includelib ..\..\c\bass.lib

  ;=============
  ; Local macros
  ;=============

  HIWORD MACRO bigword		; Retrieves the high word from double word 
    mov	eax, bigword
    shr	eax, 16			; Shift 16 for high word to set to high word
  ENDM

  HFX			TYPEDEF DWORD
  HSTREAM		TYPEDEF DWORD

  ; Declared procedures
  WinMain		PROTO
  WaitKeyCrt		PROTO

  ZeroMemory		EQU	<RtlZeroMemory>
  FillMemory		EQU	<RtlFillMemory>
  TABLESIZE		EQU	2048
  KEYS			EQU	20
  MAXVOL		EQU	4000		; higher value = longer fadeout
  FXEFFECT		EQU	9

.data 
  Keys			WORD	'Q','2','W','3','E','R','5','T','6','Y',
                                '7','U','I','9','O','0','P',219,187,221
  fxarray		BYTE	'CHORUS', 0
			BYTE    'COMPRESSOR', 0 
			BYTE    'DISTORTION', 0
			BYTE    'ECHO', 0
			BYTE    'FLANGER', 0
			BYTE    'GARGLE', 0
			BYTE    'I3DL2REVERB', 0
			BYTE    'PARAMEQ', 0
			BYTE    'REVERB', 0
Align 4
  szUseBuffer		BYTE	'using a %dms buffer', 13, 0
  szEffect		BYTE	'effect %s = %s', 9, 9, 13, 0
  szUseKeys1		BYTE	'press these keys to play:', 13, 10, 13, 10, 0
  szUseKeys2            BYTE    '  2 3  5 6 7  9 0  =', 13, 10,
                                ' Q W ER T Y UI O P[ ]', 13, 10, 13, 10, 0
  szUseKeys3            BYTE    'press -/+ to de/increase the buffer', 13, 10,
                                'press SpaceBar or Escape to quit', 13, 10, 13, 10, 0
  szDevice		BYTE	'device latency: %dms', 13, 10,
                                'device minbuf: %dms', 13, 10, 0
  
.data?
  hInstance		DWORD	?
  Wnd			DWORD	?
  Channel		DWORD	?
  Stream		HSTREAM	?
  ConsoleInput		DWORD	?
  KeyIn			INPUT_RECORD <>

  ; Strongly required, the next six lines write consistently
  TmpBuf		BYTE	128       dup(?)
  SineTable		SDWORD	TABLESIZE dup(?)	; sine table
  Vol			SDWORD	KEYS      dup(?)	; keys' volume
  Pos			SDWORD	KEYS      dup(?)	; pos
  fx			HFX	FXEFFECT  dup(?)	; effect handles
  SizeBuffers		EQU	$ - TmpBuf		; size of buffers

  fxname		DWORD	FXEFFECT  dup(?)

.code

start:

  invoke WinMain
  invoke crt__exit, 0

;-----------------------------------------------------------

  include math.inc
  
;-----------------------------------------------------------
; display error messages
;-----------------------------------------------------------

Error proc Value1 : DWORD

  invoke crt_printf, SADD("Error(%d): %s", 13, 10), rv(BASS_ErrorGetCode), Value1
  invoke BASS_Free
  invoke crt__exit, 0
  ret

Error endp

;-----------------------------------------------------------
; set color for text
;-----------------------------------------------------------
  
TextColor proc Color : DWORD
  LOCAL ScrInfo   : CONSOLE_SCREEN_BUFFER_INFO
  LOCAL StdHandle : DWORD

  mov StdHandle, rv(GetStdHandle, STD_OUTPUT_HANDLE)

  ; retrieves information about the specified console screen
  invoke GetConsoleScreenBufferInfo, StdHandle, ADDR ScrInfo
  movzx  eax, ScrInfo.wAttributes

  and eax, 0F0H
  mov ebx, Color
  and ebx, 0FH
  or  eax, ebx

  ; set the foreground (text) color attributes
  invoke SetConsoleTextAttribute, StdHandle, eax
  ret

TextColor endp

;-----------------------------------------------------------
; calculates address for literal strings
;-----------------------------------------------------------

initfxname proc

  ; init fxname table address
  lea esi, fxarray
  xor ebx, ebx
  .repeat
    lodsb
    .if al != 0
      dec esi
      mov fxname[ebx * sizeof DWORD], esi
      inc esi
      .while TRUE
        lodsb
        .break .if al == 0
      .endw
    .endif
    inc ebx
  .until ebx == FXEFFECT

  ret

initfxname endp

;-----------------------------------------------------------
  
WriteStream proc uses ebx esi edi Handle, Buffer, Len, User : DWORD
  LOCAL s : SDWORD
  LOCAL x : DWORD
  LOCAL f : float

  mov    eax, [Buffer]
  invoke ZeroMemory, eax, Len

  mov edi, Len
  shr edi, 2
  xor ebx, ebx
  .repeat
    .if !Vol[ebx * sizeof SDWORD]
      inc ebx
      .continue 
    .endif
    mov    x, ebx
    fild   x
    fadd   FP4(3.0)
    fdiv   FP4(12.0)
    fld4   2.0
    invoke Power
    fmul   FP4(901120.0)			; TABLESIZE * 440.0
    fdiv   FP4(44100.0)
    fstp   f
    xor ecx, ecx
    .repeat
      .if !Vol[ebx * sizeof SDWORD]
        inc ecx
        .continue 
      .endif
      inc   Pos[ebx * DWORD]
      fild  Pos[ebx * DWORD]
      fmul  f
      fistp x
      mov   eax, x
      and   eax, 7FFH				; TABLESIZE - 1
      fild  SineTable[eax * sizeof SDWORD]
      fild  Vol[ebx * SDWORD]
      fmul
      fdiv  FP4(4000.0)				; MAXVOL
      fistp s

      mov   esi, [Buffer]
      mov   edx, ecx
      add   edx, edx
      movsx edx, SWORD PTR [edx * sizeof SWORD + esi]
      mov   eax, s
      add   eax, edx

      .if SDWORD PTR eax > 32767
        mov eax, 32767
      .elseif SDWORD PTR eax < -32768
        mov eax, -32768
      .endif

      mov   esi, [Buffer]
      mov   edx, ecx
      add   edx, edx
      lea   edx, [edx * sizeof SWORD + esi]
      mov   WORD PTR [edx], ax
      add   edx, 2
      mov   WORD PTR [edx], ax

      .if Vol[ebx * sizeof SDWORD] < MAXVOL
        dec Vol[ebx * sizeof SDWORD]
      .endif

      inc ecx
    .until ecx == edi
    inc ebx
  .until ebx == KEYS

  return Len

WriteStream endp

;-----------------------------------------------------------
  
WinMain proc uses edi
  LOCAL Info         : BASS_INFO
  LOCAL	Temp, BufLen : DWORD

  lea edi, TmpBuf
  xor eax, eax
  mov ecx, SizeBuffers
  rep stosb

  invoke initfxname
  
  invoke crt__tzset
  invoke crt_printf, SADD('BASS Simple Sinewave Synth', 13, 10, '--------------------------', 13, 10)

  ; check the correct BASS was loaded
  invoke BASS_GetVersion
  HIWORD eax
  .if ax != BASSVERSION
    invoke crt_printf, SADD('An incorrect version of BASS.DLL was loaded', 13, 10)
    return FALSE
  .endif

  ; 10ms update period
  invoke BASS_SetConfig, BASS_CONFIG_UPDATEPERIOD, 10

  ; setup output - get latency
  invoke BASS_Init, -1, 44100, BASS_DEVICE_LATENCY, 0, 0	; BASS_Init
  .if !eax
    invoke Error, SADD('Can''t initialize device', 13, 10)
    return FALSE
  .endif

  ; build sine table
  xor ebx, ebx
  .repeat
    mov   Temp, ebx
    fild  Temp
    fldpi
    fmul
    fmul  FP8(2.0)
    fdiv  FP8(2048.0)			; TABLESIZE
    fsin
    fmul  FP8(7000.0)
    fistp SineTable[ebx * sizeof SDWORD]
    inc   ebx
  .until ebx == TABLESIZE

  invoke BASS_GetInfo, ADDR Info
  invoke crt_printf, ADDR szDevice, Info.latency, Info.minbuf
  .if Info.dsver < 8
    invoke crt_strcpy, OFFSET TmpBuf, SADD('disabled')
  .else
    invoke crt_strcpy, OFFSET TmpBuf, SADD('enabled')
  .endif
  invoke crt_printf, SADD('ds version: %d (effects %s)', 13, 10), Info.dsver, OFFSET TmpBuf

  ; default buffer size = update period + 'minbuf'
  mov    eax, Info.minbuf
  add    eax, 10
  invoke BASS_SetConfig, BASS_CONFIG_BUFFER, eax
  mov    BufLen, rv(BASS_GetConfig, BASS_CONFIG_BUFFER)

  invoke crt_printf, ADDR szUseKeys1
  invoke TextColor, FOREGROUND_GREEN + FOREGROUND_INTENSITY
  invoke crt_printf, ADDR szUseKeys2
  invoke TextColor, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE
  invoke crt_printf, ADDR szUseKeys3

  .if Info.dsver >= 8			; DX8 effects available
    invoke crt_printf, SADD('press F1-F9 to toggle effects', 13, 10, 13, 10)
  .endif

  invoke crt_printf, ADDR szUseBuffer, BufLen

  ; create a stream, stereo so that effects sound nice
  mov    Stream, rv(BASS_StreamCreate, 44100, 2, 0, WriteStream, 0)
  invoke BASS_ChannelPlay, Stream, FALSE

  .while TRUE
    invoke ReadConsoleInput, rv(GetStdHandle, STD_INPUT_HANDLE), ADDR KeyIn, 1, ADDR ConsoleInput
    .continue .if KeyIn.EventType != KEY_EVENT
    .break .if KeyIn.KeyEvent.wVirtualKeyCode == VK_SPACE || KeyIn.KeyEvent.wVirtualKeyCode == VK_ESCAPE
    .if KeyIn.KeyEvent.bKeyDown
      .if (KeyIn.KeyEvent.wVirtualKeyCode == VK_SUBTRACT || KeyIn.KeyEvent.wVirtualKeyCode == VK_ADD)
        ; recreate stream with smaller/larger buffer
        invoke BASS_StreamFree, Stream
        
        mov eax, BufLen
        .if KeyIn.KeyEvent.wVirtualKeyCode == VK_SUBTRACT
          ; smaller buffer
          dec    eax
          invoke BASS_SetConfig, BASS_CONFIG_BUFFER, eax	; BufLen - 1
        .else 
          ; larger buffer
          inc    eax
          invoke BASS_SetConfig, BASS_CONFIG_BUFFER, eax	; BufLen + 1
        .endif
 
        mov    BufLen, rv(BASS_GetConfig, BASS_CONFIG_BUFFER)
        invoke crt_printf, ADDR szUseBuffer, BufLen
        mov    Stream, rv(BASS_StreamCreate, 44100, 2, 0, WriteStream, 0)

        ; set effects on the new stream
        xor ebx, ebx
        .repeat
          .if fx[ebx * sizeof HFX]
            mov fx[ebx * sizeof HFX], rv(BASS_ChannelSetFX, Stream, ebx, 0)	; BASS_FX_DX8_CHORUS == 0
          .endif
          inc ebx
        .until ebx == 9

        invoke BASS_ChannelPlay, Stream, FALSE
      .elseif (KeyIn.KeyEvent.wVirtualKeyCode >= VK_F1 && KeyIn.KeyEvent.wVirtualKeyCode <= VK_F9)
        movzx ebx, WORD PTR KeyIn.KeyEvent.wVirtualKeyCode
        sub   ebx, VK_F1
        .if fx[ebx * sizeof HFX]
          invoke BASS_ChannelRemoveFX, Stream, fx[ebx * sizeof HFX]
          mov    fx[ebx * sizeof HFX], 0
          mov    eax, fxname[ebx * sizeof DWORD]
          invoke crt_printf, ADDR szEffect, SADD('OFF'), eax
        .else
          ; set the effect, not bothering with parameters (use defaults)
          mov fx[ebx * sizeof HFX], rv(BASS_ChannelSetFX, Stream, ebx, 0)	; BASS_FX_DX8_CHORUS == 0
          .if eax
            mov    eax, fxname[ebx * sizeof DWORD]
            invoke crt_printf, ADDR szEffect, SADD('ON'), eax
          .endif
        .endif
      .endif
    .endif

    movzx eax, WORD PTR KeyIn.KeyEvent.wVirtualKeyCode
    xor   ebx, ebx
    .repeat
      .if ax == Keys[ebx * sizeof WORD]
        .if KeyIn.KeyEvent.bKeyDown && Vol[ebx * sizeof SDWORD] != MAXVOL
          mov Pos[ebx * sizeof SDWORD], 0
          mov Vol[ebx * sizeof SDWORD], MAXVOL		; start key
        .elseif !KeyIn.KeyEvent.bKeyDown && Vol[ebx * sizeof SDWORD]
          dec Vol[ebx * sizeof SDWORD]			; trigger key fadeout
        .endif
        .break
      .endif
      inc ebx
    .until ebx == KEYS
  .endw  

  invoke BASS_Free

  ret

WinMain endp

;-----------------------------------------------------------

end start
