VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "ComDlg32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.1#0"; "mscomctl.OCX"
Begin VB.Form frmFXtest 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS DX8 effects test"
   ClientHeight    =   2535
   ClientLeft      =   45
   ClientTop       =   405
   ClientWidth     =   4470
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2535
   ScaleWidth      =   4470
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Begin VB.CheckBox chkFxToFinalMix 
      Caption         =   "Apply effects to final output instead of file"
      Height          =   375
      Left            =   240
      TabIndex        =   10
      Top             =   2040
      Width           =   3255
   End
   Begin MSComctlLib.Slider sldEQR 
      Height          =   1095
      Index           =   0
      Left            =   240
      TabIndex        =   5
      Top             =   600
      Width           =   675
      _ExtentX        =   1191
      _ExtentY        =   1931
      _Version        =   393216
      Orientation     =   1
      Max             =   20
      SelStart        =   10
      TickStyle       =   2
      TickFrequency   =   0
      Value           =   10
   End
   Begin MSComDlg.CommonDialog CMD 
      Left            =   3720
      Top             =   2040
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdOpenFP 
      Caption         =   "click here to open a file..."
      Height          =   375
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   0
      Top             =   120
      Width           =   4215
   End
   Begin MSComctlLib.Slider sldEQR 
      Height          =   1095
      Index           =   1
      Left            =   1080
      TabIndex        =   6
      Top             =   600
      Width           =   675
      _ExtentX        =   1191
      _ExtentY        =   1931
      _Version        =   393216
      Orientation     =   1
      Max             =   20
      SelStart        =   10
      TickStyle       =   2
      TickFrequency   =   0
      Value           =   10
   End
   Begin MSComctlLib.Slider sldEQR 
      Height          =   1095
      Index           =   2
      Left            =   1920
      TabIndex        =   7
      Top             =   600
      Width           =   675
      _ExtentX        =   1191
      _ExtentY        =   1931
      _Version        =   393216
      Orientation     =   1
      Max             =   20
      SelStart        =   10
      TickStyle       =   2
      TickFrequency   =   0
      Value           =   10
   End
   Begin MSComctlLib.Slider sldEQR 
      Height          =   1095
      Index           =   3
      Left            =   2760
      TabIndex        =   8
      Top             =   600
      Width           =   675
      _ExtentX        =   1191
      _ExtentY        =   1931
      _Version        =   393216
      Orientation     =   1
      Max             =   20
      SelStart        =   20
      TickStyle       =   2
      TickFrequency   =   0
      Value           =   20
   End
   Begin MSComctlLib.Slider sldEQR 
      Height          =   1095
      Index           =   4
      Left            =   3600
      TabIndex        =   9
      Top             =   600
      Width           =   675
      _ExtentX        =   1191
      _ExtentY        =   1931
      _Version        =   393216
      Orientation     =   1
      Max             =   20
      SelStart        =   20
      TickStyle       =   2
      TickFrequency   =   0
      Value           =   20
   End
   Begin VB.Label lblEQR 
      Caption         =   "volume"
      Height          =   255
      Index           =   4
      Left            =   3720
      TabIndex        =   11
      Top             =   1800
      Width           =   615
   End
   Begin VB.Label lblEQR 
      AutoSize        =   -1  'True
      Caption         =   "reverb"
      Height          =   225
      Index           =   3
      Left            =   2880
      TabIndex        =   4
      Top             =   1800
      Width           =   450
   End
   Begin VB.Label lblEQR 
      AutoSize        =   -1  'True
      Caption         =   "8 khz"
      Height          =   195
      Index           =   2
      Left            =   2040
      TabIndex        =   3
      Top             =   1800
      Width           =   480
   End
   Begin VB.Label lblEQR 
      AutoSize        =   -1  'True
      Caption         =   "1 khz"
      Height          =   195
      Index           =   1
      Left            =   1200
      TabIndex        =   2
      Top             =   1800
      Width           =   390
   End
   Begin VB.Label lblEQR 
      AutoSize        =   -1  'True
      Caption         =   "125 hz"
      Height          =   195
      Index           =   0
      Left            =   360
      TabIndex        =   1
      Top             =   1800
      Width           =   480
   End
End
Attribute VB_Name = "frmFXtest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'///////////////////////////////////////////////////////////////////////////////
' frmFXtest.frm - Copyright (c) 2001-2007 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                       [http://www.jobnik.org]
'                                                       [  jobnik@jobnik.org  ]
' Other code: mFxTest.bas
'
' BASS DX8 effects test
' Originally translated from - fxtest.c - example of Ian Luck
'///////////////////////////////////////////////////////////////////////////////

Option Explicit

Dim chan As Long         ' channel handle
Dim fx(3) As Long        ' 3 eq band + reverb

' display error messages
Public Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "error code: " & BASS_ErrorGetCode, vbExclamation, "Error")
End Sub

Private Sub Form_Initialize()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path

    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If

    ' setup output - default device
    If (BASS_Init(-1, 44100, 0, Me.hWnd, 0) = 0) Then
        Call Error_("Can't initialize device")
        End
    End If

    ' check that DX8 features are available
    Dim bi As BASS_INFO
    Call BASS_GetInfo(bi)
    If (bi.dsver < 8) Then
        Call BASS_Free
        Call Error_("DirectX 8 is not installed")
        End
    End If
    
    ' initialize effect sliders
    sldEQR(0).min = 0: sldEQR(0).max = 20
    sldEQR(0).TickFrequency = 10
    sldEQR(0).value = 10
    sldEQR(1).min = 0: sldEQR(1).max = 20
    sldEQR(1).TickFrequency = 10
    sldEQR(1).value = 10
    sldEQR(2).min = 0: sldEQR(2).max = 20
    sldEQR(2).TickFrequency = 10
    sldEQR(2).value = 10
    sldEQR(3).min = 0: sldEQR(3).max = 20
    sldEQR(3).TickFrequency = 10
    sldEQR(3).value = 20
    sldEQR(4).min = 0: sldEQR(4).max = 20
    sldEQR(4).TickFrequency = 10
    sldEQR(4).value = 10
    
End Sub

Private Sub Form_Unload(Cancel As Integer)
    If fxchan Then Call BASS_ChannelRemoveSync(fxchan, fxchansync) ' remove sync from device output stream
    Call BASS_Free
End Sub

Private Sub UpdateFX(ByVal b As Integer)
    Dim v As Integer
    v = sldEQR(b).value
    If (b < 3) Then 'EQ
        Dim p As BASS_DX8_PARAMEQ
        Call BASS_FXGetParameters(fx(b), p)
        p.fGain = 10# - v
        Call BASS_FXSetParameters(fx(b), p)
    ElseIf (b = 3) Then 'reverb
        Dim p1 As BASS_DX8_REVERB
        Call BASS_FXGetParameters(fx(3), p1)
        If v < 20 Then
            p1.fReverbMix = Log(1 - v / 20#) * 20
        Else
            p1.fReverbMix = -96
        End If
        Call BASS_FXSetParameters(fx(3), p1)
    Else 'volume
        Call BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, (20 - v) / 10!)
    End If
End Sub

Public Sub SetupFX()
    ' setup the effects
    Dim p As BASS_DX8_PARAMEQ
    Dim ch As Long

    ch = IIf(fxchan, fxchan, chan) ' set on output stream if enabled, else file stream
    fx(0) = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0) ' bass
    fx(1) = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0) ' mid
    fx(2) = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0) ' treble
    fx(3) = BASS_ChannelSetFX(ch, BASS_FX_DX8_REVERB, 0)  ' reverb

    p.fGain = 0
    p.fBandwidth = 18

    p.fCenter = 125                     ' bass   [125hz]
    Call BASS_FXSetParameters(fx(0), p)

    p.fCenter = 1000                    ' mid    [1khz]
    Call BASS_FXSetParameters(fx(1), p)

    p.fCenter = 8000                    ' treble [8khz]
    Call BASS_FXSetParameters(fx(2), p)

    Call UpdateFX(0) ' bass
    Call UpdateFX(1) ' mid
    Call UpdateFX(2) ' treble
    Call UpdateFX(3) ' reverb
End Sub

Private Sub cmdOpenFP_Click()
    CMD.filename = ""
    CMD.CancelError = True
    CMD.flags = cdlOFNExplorer Or cdlOFNFileMustExist Or cdlOFNHideReadOnly
    CMD.Filter = "playable files|*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif|All Files|*.*|"
    On Error Resume Next  ' incase Cancel is pressed
    CMD.ShowOpen
    ' if cancel was pressed, exit the procedure
    If Err.Number = 32755 Then Exit Sub
    On Error GoTo 0
    BASS_ChannelFree (chan) ' free the old channel
    chan = BASS_StreamCreateFile(BASSFALSE, StrPtr(CMD.filename), 0, 0, BASS_SAMPLE_LOOP Or BASS_SAMPLE_FLOAT)
    If (chan = 0) Then chan = BASS_MusicLoad(BASSFALSE, StrPtr(CMD.filename), 0, 0, BASS_MUSIC_LOOP Or BASS_MUSIC_RAMP Or BASS_SAMPLE_FLOAT, 1)
    If (chan = 0) Then
        cmdOpenFP.Caption = "click here to open a file..."
        Call Error_("Can't play the file")
        Exit Sub
    End If
    cmdOpenFP.Caption = GetFileName(CMD.filename)
    If Not fxchan Then Call SetupFX   ' set effects on file if not using output stream
    Call UpdateFX(4)                  ' set volume
    Call BASS_ChannelPlay(chan, BASSFALSE)
End Sub

Private Sub chkFxToFinalMix_Click()
    ' remove current effects
    Dim ch As Long
    ch = IIf(fxchan, fxchan, chan)
    Call BASS_ChannelRemoveFX(ch, fx(0))
    Call BASS_ChannelRemoveFX(ch, fx(1))
    Call BASS_ChannelRemoveFX(ch, fx(2))
    Call BASS_ChannelRemoveFX(ch, fx(3))
    If chkFxToFinalMix.value = vbChecked Then
        fxchan = BASS_StreamCreate(0, 0, 0, STREAMPROC_DEVICE, 0) ' get device output stream
        fxchansync = BASS_ChannelSetSync(fxchan, BASS_SYNC_FREE, 0, AddressOf DeviceFreeSync, 0) ' sync when device output stream is freed (format change)
    Else
        Call BASS_ChannelRemoveSync(fxchan, fxchansync) ' remove sync from device output stream
        fxchan = 0 ' stop using device output stream
    End If
    Call SetupFX
End Sub

Private Sub sldEQR_Scroll(index As Integer)
    sldEQR(index).Text = IIf(index < 3, 10 - sldEQR(index).value, 20 - sldEQR(index).value)
    Call UpdateFX(index)
End Sub

'--------------------
' useful function :)
'--------------------

' get file name from file path
Public Function GetFileName(ByVal fp As String) As String
    GetFileName = Mid(fp, InStrRev(fp, "\") + 1)
End Function
