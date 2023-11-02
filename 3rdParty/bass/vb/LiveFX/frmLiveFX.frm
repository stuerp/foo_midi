VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.1#0"; "mscomctl.OCX"
Begin VB.Form frmLiveFX 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS full-duplex example"
   ClientHeight    =   2805
   ClientLeft      =   45
   ClientTop       =   435
   ClientWidth     =   3705
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2805
   ScaleWidth      =   3705
   StartUpPosition =   2  'CenterScreen
   Begin VB.CheckBox chkFx 
      Caption         =   "Chorus"
      Height          =   195
      Index           =   3
      Left            =   1560
      TabIndex        =   8
      Top             =   2400
      Width           =   975
   End
   Begin VB.CheckBox chkFx 
      Caption         =   "Flanger"
      Height          =   195
      Index           =   2
      Left            =   240
      TabIndex        =   7
      Top             =   2400
      Width           =   975
   End
   Begin VB.CheckBox chkFx 
      Caption         =   "Gargle"
      Height          =   195
      Index           =   1
      Left            =   1560
      TabIndex        =   6
      Top             =   2040
      Width           =   975
   End
   Begin VB.ComboBox cmbInput 
      Height          =   315
      Left            =   120
      Style           =   2  'Dropdown List
      TabIndex        =   5
      Top             =   480
      Width           =   3495
   End
   Begin VB.CheckBox chkManageBuffer 
      Caption         =   "Manage buffer level"
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   1080
      Width           =   2055
   End
   Begin VB.Timer tmrLiveFX 
      Enabled         =   0   'False
      Interval        =   250
      Left            =   3120
      Top             =   1920
   End
   Begin VB.CheckBox chkFx 
      Caption         =   "Reverb"
      Height          =   195
      Index           =   0
      Left            =   240
      TabIndex        =   3
      Top             =   2040
      Width           =   975
   End
   Begin VB.ComboBox cmbDevice 
      Height          =   315
      Left            =   120
      Style           =   2  'Dropdown List
      TabIndex        =   0
      Top             =   120
      Width           =   3495
   End
   Begin MSComctlLib.Slider sLevel 
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   1560
      Width           =   3375
      _ExtentX        =   5953
      _ExtentY        =   450
      _Version        =   393216
      Max             =   100
      TickStyle       =   3
   End
   Begin VB.Label lblBuff 
      Alignment       =   2  'Center
      BorderStyle     =   1  'Fixed Single
      Height          =   315
      Left            =   2280
      TabIndex        =   2
      Top             =   1080
      Width           =   1245
   End
End
Attribute VB_Name = "frmLiveFX"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'///////////////////////////////////////////////////////////////////////////////
' frm - Copyright (c) 2002-2007 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                       [http://www.jobnik.org]
'                                                       [  jobnik@jobnik.org  ]
'
' Other source: modLiveFX.bas
'
' BASS full-duplex recording test with effects
' Originally translated from - livefx.c - Example of Ian Luck
'///////////////////////////////////////////////////////////////////////////////

Option Explicit

Private Sub chkFx_Click(n As Integer)
    If fx(n) Then
        Call BASS_ChannelRemoveFX(chan, fx(n))
        fx(n) = 0
    Else
        fx(n) = BASS_ChannelSetFX(chan, fxtype(n), n)
    End If
End Sub

Private Sub chkManageBuffer_Click()
    If chkManageBuffer.value = vbChecked Then
        buftarget = BASS_ChannelSeconds2Bytes(rchan, TARGETBUFS * info.minbuf / 1000#)  ' target buffer level
    Else
        buftarget = 0
    End If
End Sub

Private Sub cmbDevice_Change() 'device selection changed
    Dim i As Long
    i = cmbDevice.ListIndex ' get the selection
    ' initialize the selected device
    Call InitDevice(i)
End Sub

Private Sub cmbInput_Change() ' input selection changed
    Dim i As Long
    input_ = cmbInput.ListIndex ' get the selection
    While BASS_RecordSetInput(i, BASS_INPUT_OFF, -1) ' 1st disable all inputs, then...
        i = i + 1
    Wend
    Call BASS_RecordSetInput(input_, BASS_INPUT_ON, -1) ' enable the selected
End Sub

Public Function InitDevice(ByVal device As Long) As Boolean
    If chan Then BASS_StreamFree (chan) ' free output stream
    Call BASS_RecordFree ' free current device (and recording channel) if there is one
    rchan = 0
    ' initalize new device
    If BASS_RecordInit(device) = BASSFALSE Then
        Error_ ("Can't initialize recording device")
        Exit Function
    End If
    ' get list of inputs
    Dim c As Long
    Dim name As String
    cmbInput.Clear
    input_ = 0
    While BASS_RecordGetInputName(c) <> 0
        name = VBStrFromAnsiPtr(BASS_RecordGetInputName(c))
        cmbInput.AddItem name
        If (BASS_RecordGetInput(c, 0) And BASS_INPUT_OFF) = 0 Then ' this one is currently "on"
            input_ = c
            cmbInput.ListIndex = input_
        End If
        c = c + 1
    Wend
    Dim rinfo As BASS_RECORDINFO
    Call BASS_RecordGetInfo(rinfo)
    initrate = IIf(rinfo.freq, rinfo.freq, DEFAULTRATE) ' use the native recording rate (if available)
    rate = initrate
    If chan = 0 Then
        ' start output immediately to avoid needing a burst of data at start of stream
        Call BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1)
        ' initialize default output device
        If BASS_Init(-1, rate, 0, hWnd, 0) = BASSFALSE Then
            Error_ ("Can't initialize output device")
            Call BASS_RecordFree
            Exit Function
        End If
        Call BASS_GetInfo(info)
        If info.dsver < 8 Then
            ' no DX8, so disable effect buttons
            chkFx(0).Enabled = False
            chkFx(1).Enabled = False
            chkFx(2).Enabled = False
            chkFx(3).Enabled = False
        End If
    End If
    chan = BASS_StreamCreate(rate, 2, BASS_SAMPLE_FLOAT, AddressOf MyStreamProc, 0) ' create output stream
    Call BASS_ChannelSetAttribute(chan, BASS_ATTRIB_BUFFER, 0) ' disable playback buffering
    Call BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume) ' set volume level
    ' set effects
    Dim a As Long
    For a = 0 To 3
        If chkFx(a).value = vbChecked Then
            fx(a) = BASS_ChannelSetFX(chan, fxtype(a), a)
        Else
            fx(a) = 0
        End If
    Next
    
    ' don't need/want a big recording buffer
    Call BASS_SetConfig(BASS_CONFIG_REC_BUFFER, (TARGETBUFS + 3) * info.minbuf)

    ' record without a RECORDPROC so output stream can pull data from it
    rchan = BASS_RecordStart(rate, 2, BASS_SAMPLE_FLOAT, 0, 0)
    If rchan = 0 Then
        Error_ ("Can't start recording")
        Call BASS_RecordFree
        Call BASS_StreamFree(chan)
        Exit Function
    End If
    If chkManageBuffer = vbChecked Then
        buftarget = BASS_ChannelSeconds2Bytes(rchan, TARGETBUFS * info.minbuf / 1000#)  ' target buffer level
    Else
        buftarget = 0
    End If
    prebuf = True ' start prebuffering
    Call BASS_ChannelPlay(chan, False) ' start the output

    InitDevice = True
End Function

Private Sub Form_Load()
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path
    
    ' check the correct BASS was loaded
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        End
    End If

    Call MsgBox("Setting the input to the output loopback (or 'Stereo Mix' or similar on the same soundcard) " _
                & vbCrLf & "with the level set high is likely to result in nasty feedback.", vbExclamation, "Feedback warning")
                
    fxtype(0) = BASS_FX_DX8_REVERB
    fxtype(1) = BASS_FX_DX8_GARGLE
    fxtype(2) = BASS_FX_DX8_FLANGER
    fxtype(3) = BASS_FX_DX8_CHORUS
           
    ' get list of recording devices
    Dim c As Long, def As Long
    Dim di As BASS_DEVICEINFO
    While BASS_RecordGetDeviceInfo(c, di)
        cmbDevice.AddItem VBStrFromAnsiPtr(di.name)
        If di.flags And BASS_DEVICE_DEFAULT Then ' got the default device
            cmbDevice.ListIndex = c
            def = c
        End If
        c = c + 1
    Wend
    Call InitDevice(def) ' initialize default recording device
    
    tmrLiveFX.Interval = 500 ' timer to update the buffer display
    tmrLiveFX.Enabled = True
    
    volume = 0.5
    sLevel.min = 0
    sLevel.max = 100
    sLevel.value = volume * 100
End Sub

Private Sub Form_Unload(Cancel As Integer)
    tmrLiveFX.Enabled = False
    DoEvents ' give tmrLiveFX the chance to process the call
    ' release all BASS stuff
    Call BASS_RecordFree
    Call BASS_Free
End Sub

Private Sub sLevel_Scroll()
    Dim volume As Single
    volume = sLevel.value / 100#
    Call BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume)
End Sub

Private Sub tmrLiveFX_Timer()
    If rchan Then ' display current buffer level
        lblBuff.Caption = Int(BASS_ChannelBytes2Seconds(rchan, buflevel) * 1000) & " ms"
    End If
End Sub
