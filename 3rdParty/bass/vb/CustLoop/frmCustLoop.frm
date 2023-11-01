VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "ComDlg32.OCX"
Begin VB.Form frmCustLoop 
   AutoRedraw      =   -1  'True
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS custom looping example (left-click to set loop start, right-click to set end)"
   ClientHeight    =   3015
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   9000
   BeginProperty Font 
      Name            =   "Arial"
      Size            =   9.75
      Charset         =   177
      Weight          =   700
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   201
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   600
   StartUpPosition =   2  'CenterScreen
   Begin VB.Timer tmrCustLoop 
      Enabled         =   0   'False
      Interval        =   100
      Left            =   7800
      Top             =   2400
   End
   Begin MSComDlg.CommonDialog cmdCustLoop 
      Left            =   8400
      Top             =   2400
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
End
Attribute VB_Name = "frmCustLoop"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'/////////////////////////////////////////////////////////////////////////////////
' frmCustLoop.frm - Copyright (c) 2004-2007 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                         [http://www.jobnik.org]
'                                                         [  jobnik@jobnik.org  ]
'
' BASS custom looping example
' Originally translated from - custloop.c - Example of Ian Luck
'/////////////////////////////////////////////////////////////////////////////////

Option Explicit

Private Const BI_RGB = 0&
Private Const DIB_RGB_COLORS = 0&    ' color table in RGBs

Private Type BITMAPINFOHEADER
        biSize As Long
        biWidth As Long
        biHeight As Long
        biPlanes As Integer
        biBitCount As Integer
        biCompression As Long
        biSizeImage As Long
        biXPelsPerMeter As Long
        biYPelsPerMeter As Long
        biClrUsed As Long
        biClrImportant As Long
End Type

Private Type RGBQUAD
        rgbBlue As Byte
        rgbGreen As Byte
        rgbRed As Byte
        rgbReserved As Byte
End Type

Private Type BITMAPINFO
        bmiHeader As BITMAPINFOHEADER
        bmiColors(255) As RGBQUAD
End Type

Private Declare Function SetDIBitsToDevice Lib "gdi32.dll" (ByVal hdc As Long, ByVal X As Long, ByVal Y As Long, ByVal dx As Long, ByVal dy As Long, ByVal SrcX As Long, ByVal SrcY As Long, ByVal Scan As Long, ByVal NumScans As Long, Bits As Any, BitsInfo As BITMAPINFO, ByVal wUsage As Long) As Long

Private Const TRANSPARENT = 1
Private Const TA_LEFT = 0
Private Const TA_RIGHT = 2

Private Declare Function SetTextColor Lib "gdi32" (ByVal hdc As Long, ByVal crColor As Long) As Long
Private Declare Function SetBkMode Lib "gdi32" (ByVal hdc As Long, ByVal nBkMode As Long) As Long
Private Declare Function SetTextAlign Lib "gdi32" (ByVal hdc As Long, ByVal wFlags As Long) As Long
Private Declare Function TextOut Lib "gdi32" Alias "TextOutA" (ByVal hdc As Long, ByVal X As Long, ByVal Y As Long, ByVal lpString As String, ByVal nCount As Long) As Long

Private Const WIDTH_ = 600   ' display width
Private Const HEIGHT_ = 201  ' height (odd number for centre line)
Private bpp As Long          ' stream bytes per pixel
Private loop_(2) As Long     ' loop start & end
Private lsync As Long        ' looping sync
Private killscan As Boolean

Private wavebuf() As Byte    ' wave buffer
Private chan As Long         ' stream/music handle

Private bh As BITMAPINFO     ' bitmap header

' display error messages
Private Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "error code: " & BASS_ErrorGetCode, vbExclamation, "Error")
End Sub

Private Sub SetLoopStart(ByVal pos As Long)
    If loop_(1) And pos >= loop_(1) Then ' the loop end needs to move
        loop_(1) = pos + bpp
        Call BASS_ChannelSetPosition(chan, loop_(1), BASS_POS_END)
    End If
    Call BASS_ChannelSetPosition(chan, pos, BASS_POS_LOOP)
    loop_(0) = pos
End Sub

Private Sub SetLoopEnd(ByVal pos As Long)
    If pos <= loop_(0) Then ' the loop start needs to move
        loop_(0) = pos - IIf(bpp < pos, bpp, pos)
        Call BASS_ChannelSetPosition(chan, loop_(0), BASS_POS_LOOP)
    End If
    Call BASS_ChannelSetPosition(chan, pos, BASS_POS_END)
    loop_(1) = pos
End Sub

' scan the peaks
Private Sub ScanPeaks(ByVal decoder As Long)
    ReDim wavebuf(-120600 To 120600) As Byte    ' set 'n clear the buffer (600 x 201 = 120600)
    Dim cpos As Long, peak(2) As Long

    Do While (Not killscan)
        Dim Level As Long, pos As Long
        Level = BASS_ChannelGetLevel(decoder)  ' scan peaks
        pos = BASS_ChannelGetPosition(decoder, BASS_POS_BYTE) / bpp
        If (peak(0) < LoWord(Level)) Then peak(0) = LoWord(Level) ' set left peak
        If (peak(1) < HiWord(Level)) Then peak(1) = HiWord(Level) ' set right peak
        If (BASS_ChannelIsActive(decoder) = 0) Then
            pos = -1 ' reached the end
        Else
            pos = BASS_ChannelGetPosition(decoder, BASS_POS_BYTE) / bpp
        End If
        If (pos > cpos) Then
            Dim a As Long
            For a = 0 To (peak(0) * (HEIGHT_ / 2) / 32768) - 1
                ' draw left peak
                wavebuf(IIf((HEIGHT_ / 2 - 1 - a) * WIDTH_ + cpos > 120600, 120600, (HEIGHT_ / 2 - 1 - a) * WIDTH_ + cpos)) = 1 + a
            Next a
            For a = 0 To (peak(1) * (HEIGHT_ / 2) / 32768) - 1
                ' draw right peak
                wavebuf(IIf((HEIGHT_ / 2 + 1 + a) * WIDTH_ + cpos > 120600, 120600, (HEIGHT_ / 2 + 1 + a) * WIDTH_ + cpos)) = 1 + a
            Next a
            If (pos >= WIDTH_) Then Exit Do ' gone off end of display
            cpos = pos
            peak(0) = 0
            peak(1) = 0
        End If
        DoEvents
    Loop
    Call BASS_StreamFree(decoder) ' free the decoder
End Sub

' select a file to play, and start scanning it
Private Function PlayFile() As Boolean
    On Local Error Resume Next    ' if Cancel pressed...

    With frmCustLoop.cmdCustLoop
        .CancelError = True
        .flags = cdlOFNExplorer Or cdlOFNFileMustExist Or cdlOFNHideReadOnly
        .DialogTitle = "Select a file to play"
        .Filter = "Playable files|*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif;*.mo3;*.it;*.xm;*.s3m;*.mtm;*.mod;*.umx|All files|*.*"
        .ShowOpen

        ' if cancel was pressed, exit the procedure
        If Err.Number = 32755 Then Exit Function

        chan = BASS_StreamCreateFile(BASSFALSE, StrPtr(.filename), 0, 0, BASS_SAMPLE_LOOP)
        If (chan = 0) Then chan = BASS_MusicLoad(BASSFALSE, StrPtr(.filename), 0, 0, BASS_MUSIC_RAMPS Or BASS_MUSIC_POSRESET Or BASS_MUSIC_PRESCAN Or BASS_SAMPLE_LOOP, 1)

        If (chan = 0) Then
            Call Error_("Can't play file")
            PlayFile = False ' Can't load the file
            Exit Function
        End If
        
        frmCustLoop.Show   ' show form

        With bh.bmiHeader
            .biSize = Len(bh.bmiHeader)
            .biWidth = WIDTH_
            .biHeight = -HEIGHT_
            .biPlanes = 1
            .biBitCount = 8
            .biClrUsed = HEIGHT_ / 2 + 1
            .biClrImportant = HEIGHT_ / 2 + 1
        End With

        ' setup palette
        Dim a As Byte

        For a = 1 To HEIGHT_ / 2
            bh.bmiColors(a).rgbRed = (255 * a) / (HEIGHT_ / 2)
            bh.bmiColors(a).rgbGreen = 255 - bh.bmiColors(a).rgbRed
        Next a

        bpp = BASS_ChannelGetLength(chan, BASS_POS_BYTE) / WIDTH_ ' bytes per pixel
        If (bpp < BASS_ChannelSeconds2Bytes(chan, 0.02)) Then ' minimum 20ms per pixel (BASS_ChannelGetLevel scans 20ms)
            bpp = BASS_ChannelSeconds2Bytes(chan, 0.02)
        End If
        Call BASS_ChannelPlay(chan, BASSFALSE) ' start playing
        frmCustLoop.tmrCustLoop.Enabled = True ' timer's interval is 100ms (10Hz)

        Dim chan2 As Long
        chan2 = BASS_StreamCreateFile(BASSFALSE, StrPtr(.filename), 0, 0, BASS_STREAM_DECODE)
        If (chan2 = 0) Then chan2 = BASS_MusicLoad(BASSFALSE, StrPtr(.filename), 0, 0, BASS_MUSIC_DECODE, 1)
        Call ScanPeaks(chan2)    ' start scanning peaks
    End With
    PlayFile = True
End Function

Private Sub DrawTimeLine(ByVal dc As Long, ByVal pos As Long, ByVal col As Long, ByVal Y As Long)
    Dim wpos As Long
    wpos = pos / bpp
    Dim time As Long
    time = BASS_ChannelBytes2Seconds(chan, pos)
    Dim text As String
    text = time \ 60 & ":" & Format(time Mod 60, "00")
    frmCustLoop.CurrentX = wpos
    frmCustLoop.Line (wpos, 0)-(wpos, HEIGHT_ - 1), col
    Call SetTextColor(dc, col)
    Call SetBkMode(dc, TRANSPARENT)
    Call SetTextAlign(dc, IIf(wpos >= WIDTH_ / 2, TA_RIGHT, TA_LEFT))
    Call TextOut(dc, wpos, Y, text, Len(text))
End Sub

Private Sub Form_Load()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    Call ChDrive(App.Path)
    Call ChDir(App.Path)

    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If
    
    ' initialize BASS
    If (BASS_Init(-1, 44100, 0, Me.hWnd, 0) = 0) Then
        Call Error_("Can't initialize device")
        End
    End If

    If (Not PlayFile) Then ' start a file playing
        Call BASS_Free
        End
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    killscan = True
    tmrCustLoop.Enabled = False
    Call BASS_Free
    End
End Sub

Private Sub Form_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    If (Button = vbLeftButton) Then ' set loop start
        Call SetLoopStart(X * bpp)
        Call DrawTimeLine(Me.hdc, loop_(0), &HFFFF00, 12)  ' loop start
    ElseIf (Button = vbRightButton) Then    ' set loop end
        Call SetLoopEnd(X * bpp)
        Call DrawTimeLine(Me.hdc, loop_(1), vbYellow, 24) ' loop end
    End If
End Sub

Private Sub Form_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    If ((X >= 0) And (X < WIDTH_)) Then
        If (Button = vbLeftButton) Then
            Call SetLoopStart(X * bpp)
        ElseIf (Button = vbRightButton) Then
            Call SetLoopEnd(X * bpp)
        End If
    End If
End Sub

Private Sub tmrCustLoop_Timer()
    With Me
        ' draw buffered peak waveform
        Call SetDIBitsToDevice(.hdc, 0, 0, WIDTH_, HEIGHT_, 0, 0, 0, HEIGHT_, wavebuf(-(WIDTH_ / 2)), bh, 0)
        Call DrawTimeLine(.hdc, BASS_ChannelGetPosition(chan, BASS_POS_BYTE), &HFFFFFF, 0) ' current pos
        Call DrawTimeLine(.hdc, loop_(0), &HFFFF00, 12) ' loop start
        Call DrawTimeLine(.hdc, loop_(1), vbYellow, 24) ' loop end
    End With
End Sub
