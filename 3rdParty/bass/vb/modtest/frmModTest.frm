VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "ComDlg32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmModTest 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS MOD music example"
   ClientHeight    =   2115
   ClientLeft      =   2235
   ClientTop       =   2400
   ClientWidth     =   4470
   ScaleHeight     =   2115
   ScaleWidth      =   4470
   Begin VB.Timer tmrPos 
      Left            =   1560
      Top             =   720
   End
   Begin MSComDlg.CommonDialog cdlOfn 
      Left            =   2040
      Top             =   720
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin MSComctlLib.Slider sld20 
      Height          =   288
      Left            =   112
      TabIndex        =   10
      Top             =   1152
      Width           =   4246
      _ExtentX        =   7514
      _ExtentY        =   503
      _Version        =   393216
      TickStyle       =   3
   End
   Begin VB.CommandButton cmd10 
      Caption         =   "Open file..."
      Height          =   336
      Left            =   112
      TabIndex        =   0
      Top             =   120
      Width           =   4246
   End
   Begin VB.CommandButton cmd12 
      Caption         =   "Play / Pause"
      Height          =   288
      Left            =   2682
      TabIndex        =   3
      Top             =   792
      Width           =   1341
   End
   Begin VB.ComboBox cmb21 
      Height          =   1680
      Left            =   492
      Style           =   2  'Dropdown List
      TabIndex        =   5
      Top             =   1680
      Width           =   1006
   End
   Begin VB.ComboBox cmb22 
      Height          =   1680
      Left            =   1721
      Style           =   2  'Dropdown List
      TabIndex        =   7
      Top             =   1680
      Width           =   1006
   End
   Begin VB.ComboBox cmb23 
      Height          =   1680
      Left            =   2950
      Style           =   2  'Dropdown List
      TabIndex        =   9
      Top             =   1680
      Width           =   1006
   End
   Begin VB.Label lbl11 
      Alignment       =   2  'Center
      Height          =   192
      Left            =   112
      TabIndex        =   1
      Top             =   504
      Width           =   4246
   End
   Begin VB.Label lbl15 
      Alignment       =   2  'Center
      BorderStyle     =   1  'Fixed Single
      Height          =   240
      Left            =   447
      TabIndex        =   2
      Top             =   816
      Width           =   1006
   End
   Begin VB.Label lbl5 
      Alignment       =   2  'Center
      Caption         =   "Interpolation"
      Height          =   192
      Left            =   492
      TabIndex        =   4
      Top             =   1464
      Width           =   1006
   End
   Begin VB.Label lbl7 
      Alignment       =   2  'Center
      Caption         =   "Ramping"
      Height          =   192
      Left            =   1721
      TabIndex        =   6
      Top             =   1464
      Width           =   1006
   End
   Begin VB.Label lbl9 
      Alignment       =   2  'Center
      Caption         =   "Surround"
      Height          =   192
      Left            =   2950
      TabIndex        =   8
      Top             =   1464
      Width           =   1006
   End
End
Attribute VB_Name = "frmModTest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'/*
'    BASS MOD music example
'    Copyright (c) 1999-2021 Un4seen Developments Ltd.
'*/

Option Explicit

Dim music As Long
Dim sld20IsDragging As Boolean

' display error messages
Public Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "error code: " & BASS_ErrorGetCode, vbExclamation, "Error")
End Sub

Private Function GetFlags() As Long
    Dim flags As Long

    flags = BASS_MUSIC_POSRESET ' stop notes when seeking
    Select Case cmb21.ListIndex
        Case 0
            flags = flags Or BASS_MUSIC_NONINTER ' no interpolation
        Case 2
            flags = flags Or BASS_MUSIC_SINCINTER ' sinc interpolation
    End Select
    Select Case cmb22.ListIndex
        Case 1
            flags = flags Or BASS_MUSIC_RAMP ' ramping
        Case 2
            flags = flags Or BASS_MUSIC_RAMPS ' "sensitive" ramping
    End Select
    Select Case cmb23.ListIndex
        Case 1
            flags = flags Or BASS_MUSIC_SURROUND ' surround
        Case 2
            flags = flags Or BASS_MUSIC_SURROUND2 ' "mode2"
    End Select
    GetFlags = flags
End Function

Private Sub cmb21_Click()
    Call BASS_ChannelFlags(music, GetFlags(), -1&)  ' update flags
End Sub

Private Sub cmb22_Click()
    Call BASS_ChannelFlags(music, GetFlags(), -1&) ' update flags
End Sub

Private Sub cmb23_Click()
    Call BASS_ChannelFlags(music, GetFlags(), -1&) ' update flags
End Sub

Private Sub cmd10_Click()
    On Error Resume Next
    cdlOfn.ShowOpen
    If Err = cdlCancel Then Exit Sub
    On Error GoTo 0
    Call BASS_MusicFree(music) ' free the current MOD music
    music = BASS_MusicLoad(BASSFALSE, StrPtr(cdlOfn.filename), 0, 0, GetFlags() Or BASS_SAMPLE_FLOAT, 1) ' load the new MOD music
    If music Then ' success
        Dim length As Long
        length = BASS_ChannelGetLength(music, BASS_POS_MUSIC_ORDER) ' get the order length
        cmd10.Caption = cdlOfn.FileTitle
        Dim ctype As String
        Dim info As BASS_CHANNELINFO
        Dim channels As Long
        While BASS_ChannelGetAttributeEx(music, BASS_ATTRIB_MUSIC_VOL_CHAN + channels, 0, 0)
            channels = channels + 1 ' count channels
            Call BASS_ChannelGetInfo(music, info)
            Select Case info.ctype And Not BASS_CTYPE_MUSIC_MO3
                Case BASS_CTYPE_MUSIC_MOD
                    ctype = "MOD"
                Case BASS_CTYPE_MUSIC_MTM
                    ctype = "MTM"
                Case BASS_CTYPE_MUSIC_S3M
                    ctype = "S3M"
                Case BASS_CTYPE_MUSIC_XM
                    ctype = "XM"
                Case BASS_CTYPE_MUSIC_IT
                    ctype = "IT"
            End Select
        Wend
        lbl11.Caption = "name: " & VBStrFromAnsiPtr(BASS_ChannelGetTags(music, BASS_TAG_MUSIC_NAME)) & _
            ", format: " & channels & ctype & IIf(info.ctype And BASS_CTYPE_MUSIC_MO3, " (MO3)", "")
        sld20.max = length - 1 ' update scroller range
        Call BASS_ChannelPlay(music, BASSFALSE) ' start it
    Else ' failed
        cmd10.Caption = "Open file..."
        lbl11.Caption = ""
        lbl15.Caption = ""
        Call Error_("Can't play the file")
    End If
End Sub

Private Sub cmd12_Click()
    If BASS_ChannelIsActive(music) = BASS_ACTIVE_PLAYING Then
        Call BASS_ChannelPause(music)
    Else
        Call BASS_ChannelPlay(music, BASSFALSE)
    End If
End Sub

Private Sub Form_Load()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path

    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If

    ' initialize default output device
    If BASS_Init(-1, 44100, 0, hWnd, 0) = BASSFALSE Then
        Call Error_("Can't initialize device")
        End
    End If

    Const MAX_PATH = 260
    cdlOfn.MaxFileSize = MAX_PATH
    cdlOfn.flags = cdlOFNHideReadOnly Or cdlOFNExplorer
    cdlOfn.Filter = "MOD music files (mo3/xm/mod/s3m/it/mtm/umx)|*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx|All files|*.*"
    cdlOfn.CancelError = True
    
    cmb21.AddItem "off"
    cmb21.AddItem "linear"
    cmb21.AddItem "sinc"
    cmb21.ListIndex = 1
    
    cmb22.AddItem "off"
    cmb22.AddItem "normal"
    cmb22.AddItem "sensitive"
    cmb22.ListIndex = 2
    
    cmb23.AddItem "off"
    cmb23.AddItem "mode1"
    cmb23.AddItem "mode2"
    cmb23.ListIndex = 0
    
    tmrPos.Interval = 100
    tmrPos.Enabled = True
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Call BASS_Free
End Sub

Private Sub sld20_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    sld20IsDragging = True
End Sub

Private Sub sld20_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    sld20IsDragging = False
End Sub

Private Sub sld20_Scroll()
    Call BASS_ChannelSetPosition(music, sld20.value, BASS_POS_MUSIC_ORDER) ' set the position
End Sub

Private Sub tmrPos_Timer()
    ' update display
    Dim pos As Long
    pos = BASS_ChannelGetPosition(music, BASS_POS_MUSIC_ORDER)
    If pos <> -1 Then
        If Not sld20IsDragging Then sld20.value = LoWord(pos)
        lbl15.Caption = Format(LoWord(pos), "00#") & "." & Format(HiWord(pos), "00#")
    End If
End Sub
