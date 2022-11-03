comment ~
----------------------------------------------------------------
	BASS spectrum analyser example
	Copyright (c) 2002-2008 Un4seen Developments Ltd.

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
  include \masm32\include\msimg32.inc
  include \masm32\include\masm32.inc
  include \masm32\include\gdi32.inc
  include \masm32\include\user32.inc
  include \masm32\include\kernel32.inc
  include \masm32\include\Comctl32.inc
  include \masm32\include\comdlg32.inc
  include \masm32\include\winmm.inc
  include \masm32\macros\macros.asm         ; masm32 macro file

  include ..\bass.inc


; libraries
; ~~~~~~~~~~~~~~~~~~
; set here your path
; ~~~~~~~~~~~~~~~~~~
  includelib \masm32\lib\msimg32.lib
  includelib \masm32\lib\masm32.lib
  includelib \masm32\lib\gdi32.lib
  includelib \masm32\lib\user32.lib
  includelib \masm32\lib\kernel32.lib
  includelib \masm32\lib\Comctl32.lib
  includelib \masm32\lib\comdlg32.lib
  includelib \masm32\lib\winmm.lib      

  includelib ..\..\c\bass.lib

  ;=============
  ; Local macros
  ;=============

  DisplayWindow MACRO handl, ShowStyle
    invoke ShowWindow, handl, ShowStyle
    invoke UpdateWindow, handl
  ENDM

  exit MACRO
    invoke ExitProcess, 0
  ENDM

  Modulus MACRO v1 : req
    ; -> eax Value, where: Value mod v1
    mov ecx, v1 
    xor edx, edx 
    div ecx 
    EXITM <edx> 
  ENDM

  HIWORD MACRO bigword		; Retrieves the high word from double word 
    mov	eax, bigword
    shr	eax, 16			; Shift 16 for high word to set to high word
  ENDM

  float			TYPEDEF	REAL4

  ; Declared procedures
  WinMain		PROTO :DWORD,:DWORD,:DWORD
  WndProc		PROTO :DWORD,:DWORD,:DWORD,:DWORD
  TopXY			PROTO :DWORD,:DWORD

  ZeroMemory		EQU	<RtlZeroMemory>
  FillMemory		EQU	<RtlFillMemory>
  SPECWIDTH		EQU	392	; display width
  SPECHEIGHT		EQU	127	; height (changing requires palette adjustments too)
  BANDS			EQU	28

  ; mode to visualizations
  MODE_FFT		EQU	0
  MODE_LOGARITHM	EQU	1
  MODE_3D		EQU	2
  MODE_WAVE		EQU	3

.data?
  hInstance		DWORD	?
  hCursor		DWORD	?
  Wnd			DWORD	?
  WndRect		RECT	<?>

  ClockDC		DWORD	?
  ClockBmp		DWORD	?
  OldClock		DWORD	?

  NumbDC		DWORD	?
  NumbBmp		DWORD	?
  OldNumb		DWORD	?

  SpecDC		DWORD	?
  SpecBmp		DWORD	?
  OldBmp		DWORD	?
  
  SpecBuf		DWORD	?
  Timer			DWORD	?
  Channel		DWORD	?

  szFileName		BYTE	MAX_PATH dup(?)
  fft			float	1024 dup(?)	; get the FFT data
  Buf			float   ?
  ofn			OPENFILENAME <>		; structure
  ci			BASS_CHANNELINFO <>

  ; a little trick, the next two lines write consistently
  bi			BITMAPINFO <>
  pal			RGBQUAD	256 dup (<>)

.data 
  szAppName		BYTE	'BASS-Spectrum', 0
  szDisplayName		BYTE	'BASS spectrum example (click to toggle mode)', 0
  szFiles		BYTE	'playable files', 0, '*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif', 0, 'All files', 0, '*.*', 0, 0
  szOpen		BYTE	'Select a file to play', 0
  
  szError		BYTE	'Error', 0
  szNotDevice		BYTE	'Can''t initialize device', 0
  szNotPlay		BYTE	'Can''t play file', 0
  szIncorrectVersion	BYTE	'An incorrect version of BASS.DLL was loaded', 0
  
  SpecMode		DWORD	0
  SpecPos		DWORD	0

.code

start:

  invoke InitCommonControls

  mov hInstance, rv(GetModuleHandle, NULL)
  mov hCursor,   rv(LoadCursor, NULL, IDC_ARROW)

  invoke WinMain, SPECWIDTH, SPECHEIGHT, SW_SHOWDEFAULT
  exit

;-----------------------------------------------------------

  include math.inc
  
;-----------------------------------------------------------

Error proc Value1, Value2 : DWORD

  invoke MessageBox, Wnd, Value1, Value2, MB_OK or MB_ICONERROR
  ret

Error endp

;----------------------------------------------------------

AllocMem proc Bytes : DWORD

  fn     GlobalLock, rv(GlobalAlloc, GMEM_FIXED or GMEM_ZEROINIT, Bytes)
  ret

AllocMem endp

;----------------------------------------------------------

FreeMem proc hMem : DWORD

  invoke GlobalHandle, hMem
  push   eax
  invoke GlobalUnlock, eax
  call   GlobalFree
  ret

FreeMem endp

;-----------------------------------------------------------

FillChar proc uses ecx edx Dest, Len, Fill : DWORD

  invoke FillMemory, Dest, Len, Fill
  ret

FillChar endp

;-----------------------------------------------------------

TopXY proc wDim : DWORD, sDim : DWORD

  shr	 sDim, 1      ; divide screen dimension by 2
  shr	 wDim, 1      ; divide window dimension by 2
  mov	 eax, wDim    ; copy window dimension into eax
  sub	 sDim, eax    ; sub half win dimension from half screen dimension

  return sDim

TopXY endp

;-----------------------------------------------------------
; select a file to play, and play it
;-----------------------------------------------------------

PlayFile proc
  LOCAL p64 : QWORD				; for 64-bit parameters

  mov szFileName[0],	0			; set 1st byte to zero
  mov ofn.lStructSize,	sizeof OPENFILENAME
  m2m ofn.hWndOwner,	Wnd
  m2m ofn.hInstance,	hInstance
  mov ofn.nMaxFile,	MAX_PATH
  mov ofn.Flags,	OFN_FILEMUSTEXIST or OFN_HIDEREADONLY or OFN_EXPLORER
  m2m ofn.lpstrTitle,	offset szOpen
  m2m ofn.lpstrFilter,	offset szFiles             
  m2m ofn.lpstrFile,	offset szFileName

  invoke GetOpenFileName, ADDR ofn
  .if !szFileName[0]
    return FALSE
  .endif

  mov DWORD PTR [p64], 0
  mov DWORD PTR [p64 + 4], 0
  mov Channel, rv(BASS_StreamCreateFile, FALSE, ADDR szFileName, p64, p64, BASS_SAMPLE_LOOP)
  .if !eax				; if !Channel
    mov DWORD PTR [p64], 0
    mov DWORD PTR [p64 + 4], 0
    mov Channel, rv(BASS_MusicLoad, FALSE, ADDR szFileName, p64, 0, BASS_MUSIC_RAMP or BASS_SAMPLE_LOOP, 1)
    .if !eax				; if !Channel
      invoke Error, ADDR szNotPlay, ADDR szError
      return FALSE			; Can't load the file
    .endif
  .endif

  ; get number of channels
  invoke BASS_ChannelGetInfo, Channel, ADDR ci

  ; allocate buffer for data
  mov  eax, ci.chans
  imul eax, SPECWIDTH * sizeof(float)
  mov  Buf, rv(AllocMem, eax)

  invoke BASS_ChannelPlay, Channel, FALSE

  return TRUE

PlayFile endp

;-----------------------------------------------------------
; create bitmap to draw spectrum in (8 bit for easy updating)
;-----------------------------------------------------------

CreateBmp proc
  LOCAL color : DWORD

  invoke ZeroMemory, addr bi, sizeof bi + sizeof pal 
  mov bi.bmiHeader.biSize,         sizeof bi.bmiHeader
  mov bi.bmiHeader.biWidth,        SPECWIDTH
  mov bi.bmiHeader.biHeight,       SPECHEIGHT
  mov bi.bmiHeader.biPlanes,       1
  mov bi.bmiHeader.biBitCount,     8
  mov bi.bmiHeader.biClrUsed,      256
  mov bi.bmiHeader.biClrImportant, 256

  ; setup palette
  mov eax, 1
  lea esi, [pal]
  .repeat
    mov ebx, eax
    add ebx, ebx
    mov cl,  bl
    neg cl
    mov [esi.RGBQUAD].rgbGreen, cl
    mov [esi.RGBQUAD].rgbRed,   bl
    inc eax
    add esi, sizeof RGBQUAD
  .until eax == 128

  xor eax, eax
  .repeat
    mov ebx, eax
    shl ebx, 3
    mov [esi.RGBQUAD + sizeof RGBQUAD * 0].rgbBlue,  bl
    mov [esi.RGBQUAD + sizeof RGBQUAD * 32].rgbBlue, 255
    mov [esi.RGBQUAD + sizeof RGBQUAD * 32].rgbRed,  bl
    mov [esi.RGBQUAD + sizeof RGBQUAD * 64].rgbRed,  255
    mov cl, 31
    sub cl, al
    shl cl, 3
    mov [esi.RGBQUAD + sizeof RGBQUAD * 64].rgbBlue,  cl
    mov [esi.RGBQUAD + sizeof RGBQUAD * 64].rgbGreen, bl
    mov [esi.RGBQUAD + sizeof RGBQUAD * 96].rgbRed,   255
    mov [esi.RGBQUAD + sizeof RGBQUAD * 96].rgbGreen, 255
    mov [esi.RGBQUAD + sizeof RGBQUAD * 96].rgbBlue,  bl
    inc eax
    add esi, sizeof RGBQUAD
  .until eax == 32

  ; create the bitmap
  mov SpecBmp, rv(CreateDIBSection, 0, ADDR bi, DIB_RGB_COLORS, ADDR SpecBuf, NULL, 0)
  mov SpecDC,  rv(CreateCompatibleDC, 0)
  mov OldBmp,  rv(SelectObject, SpecDC, SpecBmp)

  ret

CreateBmp endp

;-----------------------------------------------------------
; fill background
;-----------------------------------------------------------

FillBackGround proc

  mov edi, [SpecBuf]
  xor ebx, ebx
  .repeat
     mov eax, 93009300H		; Index color for filling: Blue-Black-Blue-Black
     mov ecx, SPECWIDTH / 4
     rep stosd
     .break .if ebx == SPECHEIGHT / 2
     xor eax, eax		; Clear line
     mov ecx, SPECWIDTH / 4
     rep stosd
     inc ebx
  .until ebx == SPECHEIGHT / 2 + 1
  
  ret

FillBackGround endp

;-----------------------------------------------------------
; draw time
;-----------------------------------------------------------

DrawTime proc
LOCAL Len        : QWORD
LOCAL Time, mSec : DWORD
LOCAL Buffer[5]  : BYTE
LOCAL FloatTime  : float

  mov    ecx, 5
  lea    edi, [Buffer]
  rep    stosb
  
  mov    DWORD PTR [Len], 0
  mov    DWORD PTR [Len + 4], 0
  mov    DWORD PTR Len, rv(BASS_ChannelGetPosition, Channel, BASS_POS_BYTE)
  invoke BASS_ChannelBytes2Seconds, Channel, Len

  fstp   FloatTime
  fld    FloatTime
  mov    Time, rv(Trunc)	; Time of playing
  fld    FloatTime
  invoke Frac			; millisecond
  fld    FP4(1000.0)
  fmul
  fistp  mSec

  lea edi, Buffer
  mov ecx, 60
  div ecx			; eax -> Minute
  mov ebx, edx			; edx -> Second
  mov cl,  10
  div cl
  mov [edi + 0], al		; high digit
  mov [edi + 1], ah		; low digit
  mov eax, ebx
  mov cl,  10
  div cl
  mov [edi + 3], al		; high digit
  mov [edi + 4], ah		; low digit

   mov dl, 10
   .if mSec > 500		; half second
     inc dl
   .endif

  mov [edi + 2], dl		; colon

  xor ebx, ebx
  .repeat			; copy of the digit to bitmap
    mov    eax, ebx
    shl    eax, 4
    mov    cl, [edi + ebx]
    movzx  ecx, cl
    shl    ecx, 4
    invoke BitBlt, ClockDC, eax, 0, 16, 14, NumbDC, ecx, 0, SRCCOPY
    inc    ebx
  .until ebx == 5

  invoke FillBackGround
  invoke TransparentBlt, SpecDC, (SPECWIDTH - 16 * 5 * 2) / 2, (SPECHEIGHT - 14 * 2) / 2, 16 * 5 * 2, 14 * 2, ClockDC, 0, 0, 16 * 5, 14, 0

  ret

DrawTime endp

;-----------------------------------------------------------
; update the spectrum display
;-----------------------------------------------------------

UpdateSpectrum proc uTimerID, uMsg, dwUser, dw1, dw2 : DWORD
  LOCAL DC    : HDC
  LOCAL	x, sc : SDWORD
  LOCAL Temp  : QWORD

  switch SpecMode
    case MODE_WAVE
      ; waveform
      invoke DrawTime
        
      ; get the sample data (floating-point to avoid 8 & 16 bit processing)
      mov    ebx, [Buf]
      mov    eax, ci.chans
      imul   eax, SPECWIDTH * sizeof(float)
      or     eax, BASS_DATA_FLOAT
      invoke BASS_ChannelGetData, Channel, ebx, eax

      mov x, 0
      xor ebx, ebx
      .repeat
        xor esi, esi
        .repeat
          ; invert and scale to fit display
          mov  edi, [Buf]
          mov  eax, esi
          imul ci.chans
          add  eax, ebx
          fld1
          fsub float ptr [edi + eax * sizeof float]
          fmul FP4(127.0)
          fdiv FP4(2.0)
          mov  edx, rv(Trunc)

          .if SDWORD PTR edx < 0
            xor edx, edx
          .elseif SDWORD PTR edx > SPECHEIGHT
            mov edx, SPECHEIGHT - 1
          .endif

          .if !esi
            mov x, edx
          .endif

          ; draw line from previous sample...
          .repeat
            .if x < edx
              inc x
            .elseif x > edx
              dec x
            .endif

            ; left=green, right=red (could add more colours to palette for more chans)
            .if bl & 1
              mov al, 127
            .else
              mov al, 1
            .endif
            
            imul edi, x, SPECWIDTH
            add  edi, [SpecBuf]
            add  edi, esi
            stosb

          .until edx == x
          inc esi
        .until esi == SPECWIDTH
        inc ebx
      .until ebx == ci.chans

  default
    ; get the FFT data
    invoke BASS_ChannelGetData, Channel, ADDR fft, BASS_DATA_FFT2048

    switch SpecMode 
      case MODE_FFT
        ; "normal" FFT
        invoke DrawTime

        xor ebx, ebx				; variable of cycle
        xor esi, esi
        lea edi, [fft + sizeof float]
        .repeat
          ; scale it (sqrt to make low values more visible)
          fld   float ptr [edi]
          fsqrt
          fmul  FP4(3.0)
          fmul  FP4(127.0)			; SPECHEIGHT
          fsub  FP4(4.0)
          mov   ecx, rv(Trunc)

          ; cap it
          .if SDWORD PTR ecx > SPECHEIGHT
            mov ecx, SPECHEIGHT
          .endif

          ; interpolate from previous to make the display smoother
          .if ebx
            lea eax, [ecx + esi]
            sar eax, 1

            .if SIGN?
              adc eax, 0
            .endif

            dec esi
            .if SDWORD PTR esi >= 0
              .repeat
                 imul eax, esi, SPECWIDTH
                 add  eax, [SpecBuf]
                 mov  edx, ebx	; x
                 add  edx, edx
                 add  eax, edx
                 mov  edx, esi
                 inc  edx
                 mov  byte ptr [eax - 1], dl
                 dec  esi
              .until  SDWORD PTR esi < 0
            .endif
          .endif

          mov  esi, ecx
          dec esi
          .if SDWORD PTR ecx >= 0
            .repeat
               imul eax, ecx, SPECWIDTH
               add  eax, [SpecBuf]
               mov  edx, ebx	; x
               add  edx, edx
               add  eax, edx
               mov  edx, ecx
               inc  edx
               mov  byte ptr [eax], dl
               dec  ecx
            .until SDWORD PTR ecx < 0
          .endif

          add  edi, sizeof float
          inc  ebx
        .until ebx == SPECWIDTH / 2

      case MODE_LOGARITHM
        ; logarithmic, acumulate & average bins
        invoke DrawTime

        xor ebx, ebx				; variable of cycle
        xor edi, edi
        .repeat
          mov    x, ebx
          fild   x
          fmul   FP4(10.0)
          fdiv   FP4(27.0)			; BANDS - 1
          fld4   2.0
          invoke Power
          mov    ecx, rv(Trunc)

          .if SDWORD PTR ecx > 1023
            mov ecx, 1023
          .endif

          ; make sure it uses at least 1 FFT bin
          .if SDWORD PTR ecx <= edi
            mov ecx, edi
            inc ecx
          .endif

	  fldz					; Peak = 0
          .if SDWORD PTR ecx > SDWORD PTR edi
            .repeat
              inc   edi
              fld   fft[edi * sizeof float]
              fcom  st(1)			; Peak
              fstsw ax
              sahf
              .if CARRY?
                fstp st
              .else
                fstp st(1)
              .endif
            .until SDWORD PTR ecx <= SDWORD PTR edi
          .endif
          
          ; scale it (sqrt to make low values more visible)
          fsqrt
          fmul  FP4(3.0)
          fmul  FP4(127.0)			; SPECHEIGHT
          fsub  FP4(4.0)
          mov   ecx, rv(Trunc)

          ; cap it
          .if SDWORD PTR ecx > SPECHEIGHT
            mov ecx, SPECHEIGHT
          .endif

          .if SDWORD PTR ecx >= 0
            .repeat
              imul   eax, ecx, SPECWIDTH
              add    eax, SpecBuf
              imul   edx, ebx, SPECWIDTH / BANDS
              add    eax, edx
              lea    edx, [ecx + 1]
              invoke FillChar, eax, SPECWIDTH / BANDS - 2, edx
              dec    ecx
            .until  SDWORD PTR ecx < 0
          .endif
          inc ebx
        .until ebx == BANDS

      case MODE_3D
        ; "3D"
        xor ebx, ebx			; variable of cycle
        lea esi, [fft + sizeof float]
        .repeat
          ; scale it (sqrt to make low values more visible)
          fld float ptr [esi]
          fsqrt
          fmul   FP4(3.0)
          fmul   FP4(127.0)			; SPECHEIGHT
          invoke Trunc

          ; cap it
          .if SDWORD PTR eax > 127
            mov eax, 127
          .endif

          ; plot it
          add  eax, 128
          imul edi, ebx, SPECWIDTH
          add  edi, [SpecBuf]
          add  edi, SpecPos
          stosb
          add  esi, sizeof float
          inc  ebx
        .until ebx == SPECHEIGHT

        ; move marker onto next position
        mov eax, SpecPos
        inc eax
        mov SpecPos, Modulus(SPECWIDTH)

        xor ebx, ebx				; variable of cycle
        .repeat
          imul eax, ebx, SPECWIDTH
          add  eax, [SpecBuf]
          add  eax, SpecPos
          mov  byte ptr [eax], 255
          inc  ebx
        .until ebx == SPECHEIGHT
    endsw
  endsw

  ; update the display
  mov DC, rv(GetDC, Wnd)
  invoke BitBlt, DC, 0, 0, SPECWIDTH, SPECHEIGHT, SpecDC, 0, 0, SRCCOPY
  invoke ReleaseDC, Wnd, DC

  ret

UpdateSpectrum endp

;-----------------------------------------------------------

InitApp proc Win : DWORD

  ; initialize BASS
  invoke BASS_Init, -1, 44100, 0, Win, 0	; BASS_Init
  .if !eax
    invoke Error, ADDR szNotDevice, ADDR szError
    return FALSE
  .endif

  ; start a file playing
  invoke PlayFile
  .if eax == FALSE
    invoke BASS_Free
    return FALSE
  .endif

  invoke CreateBmp

  mov NumbBmp, rv(LoadBitmap, hInstance, 1000)
  mov NumbDC,  rv(CreateCompatibleDC, 0)
  mov OldNumb, rv(SelectObject, NumbDC, NumbBmp)

  mov ClockBmp, rv(CreateCompatibleBitmap, SpecDC, 16 * 5, 14)
  mov ClockDC,  rv(CreateCompatibleDC, SpecDC)
  mov OldClock, rv(SelectObject, ClockDC, ClockBmp)

  ; setup update timer (40hz)
  mov Timer, rv(timeSetEvent, 25, 25, ADDR UpdateSpectrum, 0, TIME_PERIODIC)

  return TRUE

InitApp endp

;-----------------------------------------------------------

DestroyApp proc

  .if Timer
    invoke timeKillEvent, Timer
  .endif

  invoke BASS_Free
  
  .if Buf
    invoke FreeMem, Buf
  .endif
  
  .if ClockBmp && ClockDC
    invoke DeleteObject, rv(SelectObject, ClockDC, OldClock)
    invoke DeleteDC, ClockDC
  .endif
  
  .if NumbBmp && NumbDC
    invoke DeleteObject, rv(SelectObject, NumbDC, OldNumb)
    invoke DeleteDC, NumbDC
  .endif
  
  .if SpecBmp && SpecDC
    invoke DeleteObject, rv(SelectObject, SpecDC, OldBmp)
    invoke DeleteDC, SpecDC
  .endif

  ret

DestroyApp endp

;-----------------------------------------------------------

WndProc PROC Win : DWORD, uMsg : DWORD, wParam : DWORD, lParam : DWORD
  LOCAL PS : PAINTSTRUCT
  LOCAL DC : HDC

  Switch uMsg
    case WM_CREATE
      invoke InitApp, Win
      .if eax == FALSE
        return -1
      .endif
      return 0
    
    case WM_PAINT
      invoke GetUpdateRect, Win, 0, 0
      .if eax
        mov DC, rv(BeginPaint, Win, ADDR PS)
        .if eax
          invoke BitBlt, DC, 0, 0, SPECWIDTH, SPECHEIGHT, SpecDC, 0, 0, SRCCOPY
          invoke EndPaint, Win, ADDR PS
        .endif
      .endif
      return 0
    
    case WM_LBUTTONUP
      ; swap spectrum mode
      mov eax, SpecMode
      inc eax
      mov SpecMode, Modulus(4)

;invoke SetWindowText, Wnd, str$(SpecMode)
      
      .if SpecMode == 2
        mov SpecPos, 0
      .endif
      ; clear display
      mov eax, [SpecBuf]
      invoke ZeroMemory, eax, SPECWIDTH * SPECHEIGHT
      return 0

    case WM_CLOSE
      invoke DestroyWindow, Win
      return 0

    case WM_DESTROY
      invoke DestroyApp
      invoke PostQuitMessage, 0
      return 0
  endsw

  invoke DefWindowProc, Win, uMsg, wParam, lParam
  ret

WndProc endp

;-----------------------------------------------------------

DoRegister proc
LOCAL WC : WNDCLASSEX

  ;==================================================
  ; Fill WNDCLASS structure with required variables
  ;==================================================

  invoke ZeroMemory, addr WC, sizeof WC
  mov WC.cbSize,         sizeof WNDCLASSEX
  mov WC.style,          CS_HREDRAW or CS_VREDRAW
  mrm WC.lpfnWndProc,    offset WndProc
  mrm WC.hInstance,      hInstance
  mov WC.hbrBackground,  rv(GetStockObject, BLACK_BRUSH)
  mrm WC.lpszClassName,  offset szAppName
  mrm WC.hCursor,        hCursor

  invoke RegisterClassEx, ADDR WC
  ret

DoRegister endp

;-----------------------------------------------------------
  
WinMain proc aWidth, aHeight, CmdShow : DWORD
  LOCAL msg : MSG
  LOCAL PosX, PosY : DWORD
  LOCAL dwExStyle, dwStyle : DWORD

  ; check the correct BASS was loaded
  invoke BASS_GetVersion
  HIWORD eax
  .if ax != BASSVERSION
    invoke Error, ADDR szIncorrectVersion, ADDR szError
    return FALSE
  .endif

  invoke DoRegister

  invoke SetRect, addr WndRect, 0, 0, aWidth, aHeight

  mov    dwExStyle, 0
  mov    dwStyle, WS_VISIBLE or WS_CAPTION or WS_SYSMENU or WS_MINIMIZEBOX

  invoke AdjustWindowRectEx, addr WndRect, dwStyle, FALSE, dwExStyle
  mov    eax, WndRect.left
  sub    WndRect.right, eax
  mov    eax, WndRect.top
  sub    WndRect.bottom, eax
	
  ;================================
  ; Centre window at following size
  ;================================

  mov PosX, rv(TopXY, aWidth, rv(GetSystemMetrics, SM_CXSCREEN))
  mov PosY, rv(TopXY, aHeight, rv(GetSystemMetrics, SM_CYSCREEN))

  invoke CreateWindowEx, dwExStyle, ADDR szAppName, ADDR szDisplayName,
                         dwStyle, 
                         PosX, PosY, WndRect.right, WndRect.bottom,
                         NULL, NULL, hInstance, NULL

  mov Wnd, eax

  DisplayWindow Wnd, CmdShow

  ;===================================
  ; Loop until PostQuitMessage is sent
  ;===================================

  .while TRUE
    invoke GetMessage, addr msg, NULL, 0, 0
    .break .if (!eax)
    invoke TranslateMessage, ADDR msg
    invoke DispatchMessage,  ADDR msg
  .endw

  return msg.wParam

WinMain endp

;-----------------------------------------------------------

end start
