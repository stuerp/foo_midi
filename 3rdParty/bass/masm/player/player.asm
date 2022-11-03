; #########################################################################
; Simple BASS Player in MASM32 
; by TBD
;
; MASM32 package (c) Steve 'Hutch' Hutchesson http://www.movsd.com
; BASS engine (c) Ian Luck  http://www.un4seen.com/
;
; History
;   1.1 (05.Jun.2oo2) - added VU meter
;                       added song name in status
;   1.0 (22.Mar.2oo2) - first version
; #########################################################################

      .486                      ; create 32 bit code
      .model flat, stdcall      ; 32 bit memory model
      option casemap :none      ; case sensitive

;     include files
;     ~~~~~~~~~~~~~
      include \masm32\include\windows.inc
      include \masm32\include\masm32.inc
      include \masm32\include\gdi32.inc
      include \masm32\include\user32.inc
      include \masm32\include\kernel32.inc
      include \masm32\include\Comctl32.inc
      include \masm32\include\comdlg32.inc
      include \masm32\include\winmm.inc

      include ..\bass.inc


;     libraries
;     ~~~~~~~~~
      includelib \masm32\lib\masm32.lib
      includelib \masm32\lib\gdi32.lib
      includelib \masm32\lib\user32.lib
      includelib \masm32\lib\kernel32.lib
      includelib \masm32\lib\Comctl32.lib
      includelib \masm32\lib\comdlg32.lib
      includelib \masm32\lib\winmm.lib      

      includelib ..\..\c\bass.lib
      

      WinMain          PROTO :DWORD,:DWORD,:DWORD,:DWORD
      WndProc          PROTO :DWORD,:DWORD,:DWORD,:DWORD
      TopXY            PROTO :DWORD,:DWORD
      GetFileName      PROTO :DWORD,:DWORD,:DWORD
      Do_Status        PROTO :DWORD
      RegisterWinClass PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
      MsgLoop          PROTO
      Main             PROTO
      Level            PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
      Do_ToolBar       PROTO :DWORD
      SetBmpColor      PROTO :DWORD
  
      wsprintfA PROTO C :DWORD,:VARARG
      wsprintf equ <wsprintfA>

      ;=============
      ; Local macros
      ;=============

      szText MACRO Name, Text:VARARG
        LOCAL lbl
          jmp lbl
            Name db Text,0
          lbl:
        ENDM

      m2m MACRO M1, M2
        push M2
        pop  M1
      ENDM

      return MACRO arg
        mov eax, arg
        ret
      ENDM

      DisplayMenu MACRO handl, IDnum
        invoke LoadMenu,hInstance,IDnum
        invoke SetMenu,handl,eax
      ENDM

      DisplayWindow MACRO handl, ShowStyle
        invoke ShowWindow,handl, ShowStyle
        invoke UpdateWindow,handl
      ENDM

      ; ---------------------------
      ; macros for creating toolbar
      ; ---------------------------


      TBextraData MACRO
        mov tbb.fsState,   TBSTATE_ENABLED
        mov tbb.dwData,    0
        mov tbb.iString,   0
      ENDM

      ; ------------------------------

      TBbutton MACRO bID, cID, bStyle
        mov tbb.iBitmap,   bID  ;; button  ID number
        mov tbb.idCommand, cID  ;; command ID number
        mov tbb.fsStyle,   bStyle 
        invoke SendMessage,hToolBar,TB_ADDBUTTONS,1,ADDR tbb
      ENDM

      ; ------------------------------

      TBblank MACRO
        mov tbb.iBitmap,   0
        mov tbb.idCommand, 0
        mov tbb.fsStyle,   TBSTYLE_SEP
        invoke SendMessage,hToolBar,TB_ADDBUTTONS,1,ADDR tbb
      ENDM

      ; ------------------------------

      Create_Tool_Bar MACRO Wd, Ht

        szText tbClass,"ToolbarWindow32"

        invoke CreateWindowEx,0,
                              ADDR tbClass,
                              ADDR tbClass,
                              WS_CHILD or WS_VISIBLE or TBSTYLE_FLAT,
                              0,0,500,20,
                              hWin,NULL,
                              hInstance,NULL

        mov hToolBar, eax
    
        invoke SendMessage,hToolBar,TB_BUTTONSTRUCTSIZE,sizeof TBBUTTON,0
    
        ; ---------------------------------------
        ; Put width & height of bitmap into DWORD
        ; ---------------------------------------
        mov  ecx,Wd  ;; loword = bitmap Width
        mov  eax,Ht  ;; hiword = bitmap Height
        shl  eax,16
        mov  ax, cx

        mov bSize, eax
    
        invoke SendMessage,hToolBar,TB_SETBITMAPSIZE,0,bSize
    
        invoke SetBmpColor,hTbBmp
        mov hTbBmp,eax
    
        mov tbab.hInst, 0
        m2m tbab.nID,   hTbBmp
        invoke SendMessage,hToolBar,TB_ADDBITMAP,12,ADDR tbab
    
        invoke SendMessage,hToolBar,TB_SETBUTTONSIZE,0,bSize
      ENDM


    ; --------------------------------------------
    ; equates to use directly in the message loop
    ; --------------------------------------------
        m_hWnd   equ <msg.hwnd>
        m_Msg    equ <msg.message>
        m_wParam equ <msg.wParam>
        m_lParam equ <msg.lParam>

.data?
      hInstance     dd ?
      CommandLine   dd ?
      hCursor       dd ?
      hWnd          dd ?
      ModHandle     dd ?
      ModName       dd ?
      MusicFile     dd ?         
      hStatus       dd ?
      hTbBmp        dd ?
      hToolBar      dd ?
      SurroundOn    dd ?
      timer         dd ?
      hWinLevel     dd ?
      specdc        dd ?
      specbmp       dd ?
      specbuf       dd ?
      VIS_WIDTH     = 84
      VIS_HEIGHT    = 9
      ofn           OPENFILENAME <>  ; structure
      bmp           BITMAPINFO <>
      

      
.code

; #########################################################################

start:

      invoke InitCommonControls

      invoke GetModuleHandle, NULL
      mov hInstance, eax

      invoke LoadCursor,NULL,IDC_ARROW
      mov hCursor, eax

      call Main

      invoke ExitProcess,eax

; data 
      szClassName    db "BASS_Player",0
      szDisplayName  db "Simple BASS Player",0
      szAboutText    db "BASSPlayer",13,10,"Copyright © TBD 2oo2",0
      szOpen         db "Open A File",0
      szFiles        db "MO3, MOD, S3M, IT, XM, MTM, UMX Files",0,"*.mo3;*.mod;*.s3m;*.it;*.xm;*.mtm;*.umx",0,0      
      szFormatString db "%03u:%03u | %s",0

; #########################################################################

Main proc

    LOCAL Wwd:DWORD,Wht:DWORD,Wtx:DWORD,Wty:DWORD

    invoke RegisterWinClass,ADDR WndProc,ADDR szClassName,
                       0,hCursor,COLOR_BTNFACE+1

    invoke CreateWindowEx,WS_EX_LEFT,
                          ADDR szClassName,
                          ADDR szDisplayName,
                          WS_VISIBLE or WS_CAPTION or WS_SYSMENU or WS_MINIMIZEBOX,
                          300,300,220,94,
                          NULL,NULL,
                          hInstance,NULL
    mov hWnd,eax

    DisplayMenu hWnd,600
    DisplayWindow hWnd,SW_SHOWNORMAL

    call MsgLoop
    ret

Main endp

; #########################################################################

RegisterWinClass proc lpWndProc:DWORD, lpClassName:DWORD,
                      Icon:DWORD, Cursor:DWORD, bColor:DWORD

    LOCAL wc:WNDCLASSEX

    mov wc.cbSize,         sizeof WNDCLASSEX
    mov wc.style,          CS_BYTEALIGNCLIENT or \
                           CS_BYTEALIGNWINDOW
    m2m wc.lpfnWndProc,    lpWndProc
    mov wc.cbClsExtra,     NULL
    mov wc.cbWndExtra,     NULL
    m2m wc.hInstance,      hInstance
    m2m wc.hbrBackground,  bColor
    mov wc.lpszMenuName,   NULL
    m2m wc.lpszClassName,  lpClassName
    m2m wc.hIcon,          Icon
    m2m wc.hCursor,        Cursor
    m2m wc.hIconSm,        Icon

    invoke RegisterClassEx, ADDR wc

    ret

RegisterWinClass endp

; ########################################################################

MsgLoop proc

    LOCAL msg:MSG

    StartLoop:
      invoke GetMessage,ADDR msg,NULL,0,0
      cmp eax, 0
      je ExitLoop
      invoke TranslateMessage, ADDR msg
      invoke DispatchMessage,  ADDR msg
      jmp StartLoop
    ExitLoop:

    mov eax, msg.wParam
    ret

MsgLoop endp

; #########################################################################

WndProc proc hWin   :DWORD,
             uMsg   :DWORD,
             wParam :DWORD,
             lParam :DWORD

    LOCAL var    :DWORD
    LOCAL caW    :DWORD
    LOCAL caH    :DWORD
    LOCAL Rct    :RECT
    LOCAL buffer1[128]:BYTE  ; these are two spare buffers
    LOCAL buffer2[128]:BYTE  ; for text manipulation etc..
    LOCAL szFileName[260]:BYTE
	local p64    :QWORD      ; for 64-bit parameters!!!

    .if uMsg == WM_COMMAND
    ;======== toolbar commands ========
        .if wParam == 50
            invoke BASS_ChannelPlay, ModHandle, 0    ; play tune
            test eax,eax
            jz error
            invoke SetTimer, hWin, NULL, 50, NULL ; 

        .elseif wParam == 51
            invoke BASS_ChannelStop, ModHandle

        .elseif wParam == 52
            invoke BASS_ChannelGetPosition, ModHandle, BASS_POS_MUSIC_ORDER
            .if eax != -1
                mov ebx, eax
                .while 1
                    dec ebx
                    and ebx, 0000000011111111b
					mov DWORD PTR [p64], ebx
					mov DWORD PTR [p64+4], 0
                    invoke BASS_ChannelSetPosition, ModHandle, p64, BASS_POS_MUSIC_ORDER
                .break .if eax
                .endw
            .endif

        .elseif wParam == 53
            invoke BASS_ChannelGetPosition, ModHandle, BASS_POS_MUSIC_ORDER
            .if eax !=-1
                mov ebx, eax
                .while 1
                    inc ebx
                    and ebx, 0000000011111111b
					mov DWORD PTR [p64], ebx
					mov DWORD PTR [p64+4], 0
                    invoke BASS_ChannelSetPosition, ModHandle, p64, BASS_POS_MUSIC_ORDER
                .break .if eax
                .endw
            .endif

        .elseif wParam == 54
            .if SurroundOn == FALSE
                invoke BASS_ChannelFlags, ModHandle, BASS_MUSIC_SURROUND, BASS_MUSIC_SURROUND
                mov SurroundOn, TRUE
            .else
                invoke BASS_ChannelFlags, ModHandle, 0, BASS_MUSIC_SURROUND
                mov SurroundOn, FALSE
            .endif    
        .endif

    ;======== menu commands ========

        .if wParam == 1001
            mov szFileName[0],0     ; set 1st byte to zero

            mov ofn.lStructSize, sizeof OPENFILENAME
            m2m ofn.hWndOwner,   hWnd
            m2m ofn.hInstance,   hInstance
            m2m ofn.lpstrFilter, offset szFiles             
            mov eax, ebp
            sub eax, 220h
            push eax
            pop ofn.lpstrFile
            mov ofn.nMaxFile,    sizeof szFileName
            m2m ofn.lpstrTitle,  offset szOpen
            mov ofn.Flags,       OFN_EXPLORER or OFN_FILEMUSTEXIST or \
                                 OFN_LONGNAMES
        
            invoke GetOpenFileName,ADDR ofn
            cmp szFileName[0],0     ; zero if cancel pressed in dlgbox
            je @F

            invoke BASS_MusicFree, ModHandle
            mov eax, BASS_SAMPLE_LOOP or BASS_MUSIC_POSRESET or BASS_MUSIC_RAMPS
            .if SurroundOn == TRUE
                or eax, BASS_MUSIC_SURROUND
            .endif
            mov DWORD PTR [p64], 0
            mov DWORD PTR [p64+4], 0
            invoke BASS_MusicLoad, 0, ADDR szFileName, p64, 0, eax, 1    ; load musicfile
            test eax,eax
            jz error
        
            mov [ModHandle], eax                                    ; save handle for future use
            invoke BASS_ChannelGetTags, ModHandle, BASS_TAG_MUSIC_NAME
            mov [ModName], eax

            invoke SendMessage, hWin, WM_COMMAND, 50, 0 

            @@:

        .endif

        .if wParam == 1010
            invoke SendMessage,hWin,WM_SYSCOMMAND,SC_CLOSE,NULL

        .elseif wParam == 1900
            invoke MessageBox, 0, ADDR szAboutText, ADDR szDisplayName, 0            
        .endif
    ;====== end menu commands ======
    .elseif uMsg == WM_TIMER
        invoke BASS_ChannelGetPosition, ModHandle, BASS_POS_MUSIC_ORDER
        mov edx, eax
        and eax, 0000000011111111b                      
        sar edx, 16
        invoke wsprintf, ADDR buffer1, ADDR szFormatString, eax, edx, ModName
        invoke SendMessage, hStatus, SB_SETTEXT, 0, ADDR buffer1    
                   
    .elseif uMsg == WM_CREATE
        invoke Do_ToolBar,hWin
        invoke Do_Status,hWin
        mov bmp.bmiHeader.biSize, sizeof bmp.bmiHeader
        mov bmp.bmiHeader.biWidth, VIS_WIDTH
        mov bmp.bmiHeader.biHeight, -VIS_HEIGHT
        mov bmp.bmiHeader.biPlanes, 1
        mov bmp.bmiHeader.biBitCount, 8
        mov bmp.bmiHeader.biClrUsed, VIS_WIDTH+1
        mov bmp.bmiHeader.biClrImportant, VIS_WIDTH+1
        invoke GetSysColor,COLOR_BTNFACE
        mov bmp.bmiColors, eax                   ; change from RGB to BGR
        mov cl, bmp.bmiColors.rgbRed
        mov bmp.bmiColors.rgbBlue, cl
        mov bmp.bmiColors.rgbRed, al
        mov eax, 1
        mov ebx, 255
        .while eax <= VIS_WIDTH ; create pallete
         mov bmp.bmiColors[eax*sizeof bmp.bmiColors].rgbRed, 0
         mov bmp.bmiColors[eax*sizeof bmp.bmiColors].rgbGreen, bl
         mov bmp.bmiColors[eax*sizeof bmp.bmiColors].rgbBlue, 0
         sub ebx, 2
         inc eax
        .endw
        invoke CreateDIBSection, 0, ADDR bmp, DIB_RGB_COLORS, ADDR specbuf, NULL, 0
        mov specbmp, eax
        invoke CreateCompatibleDC, 0
        mov specdc, eax
        invoke SelectObject, specdc, specbmp

        push hWin
        pop  hWinLevel
        invoke timeSetEvent, 25, 25, ADDR Level, 0, TIME_PERIODIC
        mov timer, eax

   
        invoke BASS_Init, -1, 44100, 0, hWin, 0          ; BASS_Init
        test eax,eax                                            
        jz error                                                
        							
        invoke BASS_Start                                       ; start BASS engine
        test eax,eax
        jz error
        
        mov SurroundOn, FALSE

    .elseif uMsg == WM_SYSCOLORCHANGE
        invoke Do_ToolBar,hWin
     
    .elseif uMsg == WM_KEYDOWN
        .IF wParam == VK_LEFT
            invoke SendMessage, hWin, WM_COMMAND, 52, 0 
        .ELSEIF wParam == VK_RIGHT        
            invoke SendMessage, hWin, WM_COMMAND, 53, 0 
        .ENDIF    

    .elseif uMsg == WM_SIZE                                    ; needed to update the status
        invoke SendMessage,hToolBar,TB_AUTOSIZE,0,0
        invoke MoveWindow,hStatus,0,0,0,0,TRUE

    .elseif uMsg == WM_CLOSE
    .elseif uMsg == WM_DESTROY
        invoke timeKillEvent, timer
        invoke KillTimer, hWin, NULL
        invoke BASS_Free
        .if hTbBmp != NULL  
          invoke DeleteObject, hTbBmp  
        .endif
        .if specbmp != NULL 
          invoke DeleteObject, specbmp 
        .endif
        .if specdc != NULL  
          invoke DeleteDC, specdc      
        .endif          
        invoke PostQuitMessage,NULL        
        return 0

    .endif

    invoke DefWindowProc,hWin,uMsg,wParam,lParam
error:
    ret

WndProc endp

; ##########################################################################

Do_ToolBar proc hWin :DWORD

    LOCAL bSize :DWORD
    LOCAL tbab  :TBADDBITMAP
    LOCAL tbb   :TBBUTTON

    invoke LoadBitmap,hInstance,750
    mov hTbBmp,eax

    Create_Tool_Bar 16, 16

    TBextraData     ; additional data for TBBUTTON structure

    TBblank
    TBbutton  0,  50, TBSTYLE_BUTTON
    TBbutton  1,  51, TBSTYLE_BUTTON
    TBbutton  2,  52, TBSTYLE_BUTTON
    TBbutton  3,  53, TBSTYLE_BUTTON
    TBbutton  4,  54, TBSTYLE_CHECK

    ret

Do_ToolBar endp

; #########################################################################

Do_Status proc hParent:DWORD

    LOCAL sbParts[ 4] :DWORD

    invoke CreateStatusWindow,WS_CHILD or WS_VISIBLE,NULL, hParent, 200
    mov hStatus, eax

    ret

Do_Status endp

; ########################################################################

SetBmpColor proc hBitmap:DWORD

    LOCAL mDC       :DWORD
    LOCAL hBrush    :DWORD
    LOCAL hOldBmp   :DWORD
    LOCAL hReturn   :DWORD
    LOCAL hOldBrush :DWORD

    invoke CreateCompatibleDC,NULL
    mov mDC,eax

    invoke SelectObject,mDC,hBitmap
    mov hOldBmp,eax

    invoke GetSysColor,COLOR_BTNFACE
    invoke CreateSolidBrush,eax
    mov hBrush,eax

    invoke SelectObject,mDC,hBrush
    mov hOldBrush,eax

    invoke GetPixel,mDC,1,1   
    invoke ExtFloodFill,mDC,1,1,eax,FLOODFILLSURFACE

    invoke SelectObject,mDC,hOldBrush
    invoke DeleteObject,hBrush

    invoke SelectObject,mDC,hBitmap
    mov hReturn,eax
    invoke DeleteDC,mDC

    mov eax,hReturn

    ret

SetBmpColor endp

; ##################################################################################

Level proc uses ebx, p1,p2,p3,p4,p5 :DWORD

    LOCAL hDC         :DWORD
    LOCAL buffer1[12] :BYTE

    .if ModHandle != 0
       invoke GetDC, hWinLevel
       mov hDC, eax

       mov ecx, [specbuf]                             ; clear window
       xor ebx, ebx
       .while ebx < VIS_WIDTH * VIS_HEIGHT
         mov BYTE PTR [ebx+ecx], 0
         inc ebx
       .endw
       
       invoke BASS_ChannelGetLevel, ModHandle
       mov ecx, [specbuf]
       .if eax != -1                                  ; error?
        .if ah > VIS_WIDTH                            ; level exceed VIS_WIDTH
          mov ah, VIS_WIDTH 
        .endif 
        xor ebx, ebx
        .while bl < ah                                ; display level
          mov BYTE PTR [ebx+ecx], bl                  ; line 1
          mov BYTE PTR [ebx+ecx+VIS_WIDTH], bl        ; line 2
          mov BYTE PTR [ebx+ecx+VIS_WIDTH*2], bl      ; line 3              
          mov BYTE PTR [ebx+ecx+VIS_WIDTH*3], bl      ; line 4                        
          inc bl        
        .endw
		ror eax,16         ; get right channel
		add ecx,VIS_WIDTH*5
        .if ah > VIS_WIDTH                            ; level exceed VIS_WIDTH
          mov ah, VIS_WIDTH 
        .endif 
        xor ebx, ebx
        .while bl < ah                                ; display level
          mov BYTE PTR [ebx+ecx], bl                  ; line 1
          mov BYTE PTR [ebx+ecx+VIS_WIDTH], bl        ; line 2
          mov BYTE PTR [ebx+ecx+VIS_WIDTH*2], bl      ; line 3              
          mov BYTE PTR [ebx+ecx+VIS_WIDTH*3], bl      ; line 4                        
          inc bl        
        .endw
       .endif 
       invoke BitBlt, hDC, 125, 9, VIS_WIDTH,VIS_HEIGHT, specdc, 0, 0, SRCCOPY
       invoke ReleaseDC, hWinLevel, hDC
    .endif
    
    ret

Level endp

; ##################################################################################

end start

; ############################################################### [ END ] ##########

