comment ~
---------------------------------------------------------------------
	BASS internet radio example
	Copyright (c) 2002-2008 Un4seen Developments Ltd.

        C++ to MASM adapted by Evgeny Melnikov

        http://dsoft1961.narod.ru
        mail angvelem@gmail.com

        WARNING: do not use national character set in "Custom URL"
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

  HIWORD MACRO bigword			; Retrieves the high word from double word 
    mov	eax, bigword
    shr	eax, 16				; Shift 16 for high word to set to high word
  ENDM

  MESS MACRO args : VARARG
    arg equ <invoke SendDlgItemMessage>	;; construct invoke and function name
    FOR var, <args>			;; loop through all arguments
      arg CATSTR arg, <, reparg(var)>   ;; replace quotes and append arg
    ENDM
    arg					;; write the invoke macro
  ENDM

  HSYNC			TYPEDEF DWORD	; synchronizer handle
  float			TYPEDEF	REAL4

  ; Declared procedures
  WinMain		PROTO
  WndProc		PROTO :DWORD,:DWORD,:DWORD,:DWORD
  NetTimer		PROTO :DWORD,:DWORD,:DWORD,:DWORD

.data
  szError		BYTE	'Error', 0
  szNotDevice		BYTE	'Can''t initialize device', 0
  szNotStream		BYTE	'Can''t play the stream', 0
  szIncorrectVersion	BYTE	'An incorrect version of BASS.DLL was loaded', 0
  szNotPlaying		BYTE	'not playing', 0

  ; preset stream URLs
  UrlArray		BYTE	'http://www.radioparadise.com/musiclinks/rp_128-9.m3u', 0
			BYTE	'http://www.radioparadise.com/musiclinks/rp_32.m3u', 0
			BYTE	'http://ogg2.as34763.net/vr160.ogg', 0
			BYTE	'http://ogg2.as34763.net/vr32.ogg', 0
			BYTE	'http://ogg2.as34763.net/a8160.ogg', 0
			BYTE	'http://ogg2.as34763.net/a832.ogg', 0
			BYTE	'http://somafm.com/secretagent.pls', 0
			BYTE	'http://somafm.com/secretagent24.pls', 0
			BYTE	'http://somafm.com/suburbsofgoa.pls', 0
			BYTE	'http://somafm.com/suburbsofgoa24.pls', 0

.data?
  hInstance		DWORD	?
  hCursor		DWORD	?
  Wnd			DWORD	?
  hThread		DWORD	?
  Channel		DWORD	?

  Urls			DWORD	10	dup(?)
  Proxy			BYTE	100	dup(?)	; proxy server

  ti			TOOLINFO <>
  hToolTip		DWORD	?
  hHook			DWORD	?

.code

start:

  invoke InitCommonControls

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

EnumChildProc proc WndCtrl, lParam : DWORD
  LOCAL szClass[255] : BYTE

  invoke GetClassName, WndCtrl, ADDR szClass, sizeof szClass
  mov    ebx, rv(crt__strcmpi, ADDR szClass, SADD('BUTTON'))
  .if !ebx || rv(crt__strcmpi, ADDR szClass, SADD('EDIT')) == 0
    mov ti.cbSize,      sizeof TOOLINFO
    mov ti.uFlags,      TTF_IDISHWND
    mov ti.hWnd,        rv(GetParent, WndCtrl)
    mrm ti.uId,         WndCtrl
    mrm ti.hInst,       hInstance
    mov ti.lpszText,    LPSTR_TEXTCALLBACK
    invoke SendMessage, lParam, TTM_ADDTOOL, 0, ADDR ti
  .endif

  return TRUE

EnumChildProc endp

;-----------------------------------------------------------

CreateToolTip proc
  LOCAL aRect : RECT

  invoke CreateWindowEx, 0, SADD('tooltips_class32'), 0, TTS_ALWAYSTIP,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         Wnd, 0, hInstance, 0

  mov ebx, eax
  .if ebx
    mov ti.cbSize, sizeof TOOLINFO
    mov ti.uFlags, TTF_IDISHWND
    mrm ti.hWnd,   Wnd

    invoke SendMessage, ebx, TTM_GETMARGIN, 0, ADDR aRect
    lea    eax, aRect
    add    [eax.RECT].left, 2
    add    [eax.RECT].top, 2
    add    [eax.RECT].right, 2
    add    [eax.RECT].bottom, 2
    invoke SendMessage, ebx, TTM_SETMARGIN, 0, eax
    invoke SendMessage, ebx, TTM_SETTIPBKCOLOR, 0D0DEE8H, 0
  .endif

  invoke EnumChildWindows, Wnd, EnumChildProc, ebx

  return ebx

CreateToolTip endp

;-----------------------------------------------------------

HookProc proc nCode, wParam, lParam : DWORD
  LOCAL msg : MSG

  mov ebx, lParam
  mov eax, [ebx.MSG].message

  .if (nCode >= 0) && ((eax == WM_MOUSEMOVE) || (eax == WM_NCLBUTTONDOWN) || \
                       (eax == WM_LBUTTONUP) || (eax == WM_RBUTTONUP) || \
                       (eax == WM_RBUTTONDOWN))

    invoke SendMessage, hToolTip, TTM_RELAYEVENT, 0, lParam
  .endif

  invoke CallNextHookEx, hHook, nCode, wParam, lParam
 
  ret

HookProc endp

;-----------------------------------------------------------
; update stream title from metadata
;-----------------------------------------------------------

DoMeta proc
  LOCAL temp[200]       : BYTE
  LOCAL _Artist, _Title : DWORD

  invoke BASS_ChannelGetTags, Channel, BASS_TAG_META
  ; got Shoutcast metadata
  .if eax						; meta
    mov ebx, rv(crt_strstr, eax, SADD("StreamTitle='"))
    .if ebx						; p
      lea    eax, [ebx + 13]
      mov    ebx, rv(crt__strdup, eax)
      invoke crt_strchr, ebx, ';'
      mov    BYTE PTR [eax - 1], 0
      MESS   Wnd, 30, WM_SETTEXT, 0, ebx
      invoke crt_free, ebx
    .endif
  .else
    invoke BASS_ChannelGetTags, Channel, BASS_TAG_OGG
    ; got Icecast/OGG tags
    .if eax						; meta
      mov ebx, eax					; p
      mov _Artist, 0
      mov _Title, 0
      .repeat
        mov esi, rv(lstrlen, ebx)
        dec esi
        ; found the artist
        .if !rv(crt__strnicmp, ebx, SADD('artist='), 7)
          mov ecx, ebx
          add ecx, 7
          mov _Artist, ecx
          add ebx, esi
        .endif
        ; found the title
        .if !rv(crt__strnicmp, ebx, SADD('title='), 6)
          mov ecx, ebx
          add ecx, 6
          mov _Title, ecx
          add ebx, esi
        .endif
      .until BYTE PTR [ebx] == 0 && BYTE PTR [ebx + 1] == 0

      .if _Artist
        invoke crt__snprintf, ADDR temp, sizeof temp, SADD('%s - %s'), _Artist, _Title
        MESS   Wnd, 30, WM_SETTEXT, 0, ADDR temp
      .elseif _Title
        MESS Wnd, 30, WM_SETTEXT, 0, _Title
      .endif
    .endif
  .endif

  ret

DoMeta endp

;-----------------------------------------------------------

MetaSync proc handle : HSYNC, channel, data, user : DWORD

  invoke DoMeta
  ret

MetaSync endp

;-----------------------------------------------------------

EndSync proc handle : HSYNC, channel, data, user : DWORD

  MESS Wnd, 30, WM_SETTEXT, 0, 0
  MESS Wnd, 31, WM_SETTEXT, 0, ADDR szNotPlaying
  MESS Wnd, 32, WM_SETTEXT, 0, 0
  ret

EndSync endp

;-----------------------------------------------------------

StatusProc proc Buffer, Len, User : DWORD

  .if Buffer && !Len
    mov  eax, [Buffer]
    MESS Wnd, 32, WM_SETTEXT, 0, eax		; display connection status
  .endif

  ret

StatusProc endp

;-----------------------------------------------------------

OpenURL proc url : DWORD

  invoke KillTimer, Wnd, 0			; stop prebuffer monitoring
  invoke BASS_StreamFree, Channel		; close old stream
  MESS Wnd, 30, WM_SETTEXT, 0, 0
  MESS Wnd, 31, WM_SETTEXT, 0, SADD('connecting...')
  MESS Wnd, 32, WM_SETTEXT, 0, 0
  ; open URL
  mov Channel, rv(BASS_StreamCreateURL, url, 0, BASS_STREAM_BLOCK + BASS_STREAM_STATUS + BASS_STREAM_AUTOFREE, StatusProc, 0);BASS_UNICODE
  ; free temp URL buffer
  invoke crt_free, url
  .if !Channel					; failed to open
    MESS   Wnd, 31, WM_SETTEXT, 0, ADDR szNotPlaying
    invoke Error, ADDR szNotStream, ADDR szError
  .else
    invoke SetTimer, Wnd, 0, 50, NetTimer	; start prebuffer monitoring
  .endif

  mov hThread, 0
  ret

OpenURL endp

;-----------------------------------------------------------

NetTimer proc aWnd, uMsg, idEvent, dwTime : DWORD
  LOCAL temp[200] : BYTE
  LOCAL Progress  : DWORD
  LOCAL p64       : QWORD			; for 64-bit parameters!!!

  ; monitor prebuffering progress
  mov  ebx, rv(BASS_StreamGetFilePosition, Channel, BASS_FILEPOS_END)
  mov  eax, rv(BASS_StreamGetFilePosition, Channel, BASS_FILEPOS_BUFFER)
  imul eax, 100
  ; percentage of buffer filled
  idiv ebx
  mov  Progress, eax

  invoke BASS_StreamGetFilePosition, Channel, BASS_FILEPOS_CONNECTED
  ;  over 75% full (or end of download)
  .if Progress > 75 || !eax
    ; finished prebuffering, stop monitoring
    invoke KillTimer, aWnd, 0

    ; get the broadcast name and URL
    mov ebx, rv(BASS_ChannelGetTags, Channel, BASS_TAG_ICY)
    .if !ebx					; icy
      ; no ICY tags, try HTTP
      mov ebx, rv(BASS_ChannelGetTags, Channel, BASS_TAG_HTTP)
    .endif
    .if ebx					; icy
      .repeat
        .if !rv(crt__strnicmp, ebx, SADD('icy-name:'), 9)
          lea  eax, [ebx + 9]
          MESS Wnd, 31, WM_SETTEXT, 0, eax
        .endif
        .if !rv(crt__strnicmp, ebx, SADD('icy-url:'), 8)
          lea  eax, [ebx + 8]
          MESS aWnd, 32, WM_SETTEXT, 0, eax
        .endif
        inc ebx
        lea eax, [ebx]
      .until BYTE PTR [eax] == 0 && BYTE PTR [eax + 1] == 0
    .else
      MESS aWnd, 31, WM_SETTEXT, 0, 0
    .endif

    mov DWORD PTR [p64], 0
    mov DWORD PTR [p64 + 4], 0
    ; get the stream title and set sync for subsequent titles
    invoke DoMeta
    ; Shoutcast
    invoke BASS_ChannelSetSync, Channel, BASS_SYNC_META, p64, MetaSync, 0
    ; Icecast/OGG
    invoke BASS_ChannelSetSync, Channel, BASS_SYNC_OGG_CHANGE, p64, MetaSync, 0
    ; set sync for end of stream
    invoke BASS_ChannelSetSync, Channel, BASS_SYNC_END, p64, EndSync, 0
    ; play it!
    invoke BASS_ChannelPlay, Channel, FALSE
    MESS aWnd, 31, WM_SETTEXT, 0, SADD('playing')
  .else
    invoke crt_sprintf, ADDR temp, SADD('buffering... %d%%'), Progress
    MESS aWnd, 31, WM_SETTEXT, 0, ADDR temp
  .endif

  ret

NetTimer endp

;-----------------------------------------------------------
; calculates address for literal strings
;-----------------------------------------------------------

initurlname proc

  ; init Urls table address
  lea esi, UrlArray
  xor ebx, ebx
  .repeat
    mov Urls[ebx * sizeof DWORD], esi
    .repeat
      lodsb
    .until al == 0
    inc ebx
  .until ebx == 10
  
  ret

initurlname endp

;-----------------------------------------------------------

InitApp proc

  mov    hThread, 0
  invoke initurlname

  ; setup output device
  .if !rv(BASS_Init, -1, 44100, 0, Wnd, NULL)
    invoke Error, ADDR szNotDevice, szError
    return FALSE
  .endif

  ; enable playlist processing
  invoke BASS_SetConfig, BASS_CONFIG_NET_PLAYLIST, 1
  ; minimize automatic pre-buffering, so we can do it
  ; (and display it) instead
  invoke BASS_SetConfig, BASS_CONFIG_NET_PREBUF, 0
  ; setup proxy server location
  invoke BASS_SetConfigPtr, BASS_CONFIG_NET_PROXY, ADDR Proxy

  mov    hToolTip, rv(CreateToolTip)
  mov    hHook, rv(SetWindowsHookEx, WH_GETMESSAGE, HookProc, 0, rv(GetCurrentThreadId))

  return TRUE

InitApp endp

;-----------------------------------------------------------

DialogProc proc Win, uMsg, wParam, lParam : DWORD
  LOCAL temp[200] : BYTE
  LOCAL url       : DWORD

  Switch uMsg
    case WM_INITDIALOG
      mrm    Wnd, Win
      invoke InitApp
      .if eax == FALSE
        invoke SendMessage, Win, WM_CLOSE, 0, 0
        return FALSE
      .endif
      return TRUE

      case WM_NOTIFY
        mov ebx, lParam
        lea ecx, [ebx.TOOLTIPTEXT].lpszText
        mov BYTE PTR [temp], 0
        .if [ebx.TOOLTIPTEXT].hdr.code == TTN_NEEDTEXT
          invoke GetDlgCtrlID, [ebx.TOOLTIPTEXT].hdr.idFrom
          Switch eax
            case 20
              invoke crt_sprintf, ADDR temp, SADD('Input custom path')
            case 21
              invoke crt_sprintf, ADDR temp, SADD('Playing custom path')
            case 40
              invoke crt_sprintf, ADDR temp, SADD('Input proxy address')
            case 41
              invoke crt_sprintf, ADDR temp, SADD('Use proxy')
            default
              .if eax >= 10 && eax < 20
      	        sub    eax, 10
                mov    eax, Urls[eax * sizeof DWORD]
                invoke crt_sprintf, ADDR temp, eax
              .endif
          endsw
          
          .if BYTE PTR [temp]
            lea eax, temp
            mov [ebx.TOOLTIPTEXT].lpszText, eax
  	  .endif
        .endif

      case WM_COMMAND
        LOWORD wParam
      	Switch eax
      	  case IDCANCEL
            invoke SendMessage, Win, WM_CLOSE, 0, 0
      	    return TRUE

      	  case 41
      	    MESS Win, 41, BM_GETCHECK, 0, 0
      	    .if eax
      	      ; disable proxy
      	      invoke BASS_SetConfigPtr, BASS_CONFIG_NET_PROXY, NULL
      	    .else
      	      ; enable proxy
      	      invoke BASS_SetConfigPtr, BASS_CONFIG_NET_PROXY, ADDR Proxy
      	    .endif

      	  default
            .if (eax >= 10 && eax < 20) || eax == 21
      	      .if hThread		; already connecting
                invoke MessageBeep, 0
              .endif
              ; get proxy server
              invoke GetDlgItemText, Win, 40, ADDR Proxy, sizeof Proxy - 1
              LOWORD wParam
              ; custom stream URL
              .if eax == 21
                MESS   Win, 20, WM_GETTEXT, sizeof temp, ADDR temp
      	        mov    url, rv(crt__strdup, ADDR temp)
      	      .else			; preset
      	        sub eax, 10
                mov eax, Urls[eax * sizeof DWORD]
      	        mov url, rv(crt__strdup, eax)
      	      .endif

      	      mov eax, [url]

      	      .if BYTE PTR [eax]
                ; open URL in a new thread (so that main thread is free)
                mov hThread, rv(crt__beginthread, OpenURL, 0, url)
              .else
                invoke crt_free, url
                invoke Error, SADD('Address line, can not be empty'), ADDR szError
              .endif
            .endif
        endsw

    case WM_CLOSE
      invoke UnhookWindowsHookEx, hHook
      invoke DestroyWindow, hToolTip
      invoke BASS_Free
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
