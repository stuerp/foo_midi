VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "ComDlg32.OCX"
Begin VB.Form frmMulti 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "BASS multiple output example"
   ClientHeight    =   2430
   ClientLeft      =   45
   ClientTop       =   435
   ClientWidth     =   5115
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2430
   ScaleWidth      =   5115
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdSwap 
      Caption         =   "Swap"
      Height          =   375
      Left            =   4260
      TabIndex        =   2
      Top             =   1080
      Width           =   555
   End
   Begin VB.Frame frameMulti 
      Caption         =   " Device 2"
      Height          =   1095
      Index           =   1
      Left            =   120
      TabIndex        =   5
      Top             =   1260
      Width           =   4875
      Begin VB.CommandButton cmdOpen 
         Caption         =   "Open file..."
         Height          =   375
         Index           =   1
         Left            =   120
         TabIndex        =   8
         Top             =   585
         Width           =   3585
      End
      Begin VB.CommandButton cmdClone 
         Caption         =   "Clone #1"
         Height          =   375
         Index           =   1
         Left            =   3795
         TabIndex        =   7
         Top             =   585
         Width           =   930
      End
      Begin VB.ComboBox cmbDevice 
         Height          =   315
         Index           =   1
         Left            =   120
         Style           =   2  'Dropdown List
         TabIndex        =   6
         Top             =   240
         Width           =   4635
      End
   End
   Begin VB.Frame frameMulti 
      Caption         =   " Device 1 "
      Height          =   1095
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   60
      Width           =   4875
      Begin VB.ComboBox cmbDevice 
         Height          =   315
         Index           =   0
         Left            =   120
         Style           =   2  'Dropdown List
         TabIndex        =   4
         Top             =   240
         Width           =   4635
      End
      Begin VB.CommandButton cmdClone 
         Caption         =   "Clone #2"
         Height          =   375
         Index           =   0
         Left            =   3795
         TabIndex        =   3
         Top             =   585
         Width           =   930
      End
      Begin VB.CommandButton cmdOpen 
         Caption         =   "Open file..."
         Height          =   375
         Index           =   0
         Left            =   120
         TabIndex        =   1
         Top             =   585
         Width           =   3585
      End
   End
   Begin MSComDlg.CommonDialog cmd 
      Left            =   4455
      Top             =   1860
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
End
Attribute VB_Name = "frmMulti"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'//////////////////////////////////////////////////////////////////////////////
' frmMulti.frm - Copyright (c) 2003-2007 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                      [http://www.jobnik.org]
'                                                      [  jobnik@jobnik.org  ]
' Other sources: frmDevice.frm and modMulti.bas
'
' BASS Multiple output example
' Originally translated from - multi.c - Example of Ian Luck
'//////////////////////////////////////////////////////////////////////////////
 
Option Explicit

Dim outdev(1) As Long   ' output devices
Dim chan(1) As Long     ' the streams

' display error messages
Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "Error Code : " & BASS_ErrorGetCode, vbExclamation, "Error")
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

    cmd.CancelError = True
    cmd.flags = cdlOFNExplorer Or cdlOFNHideReadOnly
    cmd.Filter = "streamable files|*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif|All files|*.*"

    ' get list of output devices
    outdev(0) = 1
    outdev(1) = 0
    Dim c As Long
    Dim di As BASS_DEVICEINFO
    While BASS_GetDeviceInfo(c, di)
       cmbDevice(0).AddItem VBStrFromAnsiPtr(di.name)
       If c = outdev(0) Then cmbDevice(0).ListIndex = c
       cmbDevice(1).AddItem VBStrFromAnsiPtr(di.name)
       If c = outdev(1) Then cmbDevice(1).ListIndex = c
        c = c + 1
    Wend
    ' initialize the output devices
    If (BASS_Init(outdev(0), 44100, 0, hWnd, 0) = BASSFALSE) Or (BASS_Init(outdev(1), 44100, 0, hWnd, 0) = BASSFALSE) Then
        Call Error_("Can't initialize device")
        Unload Me
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    ' release both devices
    Call BASS_SetDevice(outdev(0))
    Call BASS_Free
    Call BASS_SetDevice(outdev(1))
    Call BASS_Free
    End
End Sub

Private Sub cmdOpen_Click(devn As Integer)
    On Error Resume Next ' if Cancel pressed...
    cmd.ShowOpen  ' open a file to play on selected device
    If Err.Number = 32755 Then Exit Sub ' if cancel was pressed, exit the procedure
    On Error GoTo 0
    Call BASS_StreamFree(chan(devn)) ' free the old channel
    Call BASS_SetDevice(outdev(devn)) ' set the device to create stream on
    chan(devn) = BASS_StreamCreateFile(BASSFALSE, StrPtr(cmd.filename), 0, 0, BASS_SAMPLE_LOOP Or BASS_SAMPLE_FLOAT)
    If (chan(devn) = 0) Then
        chan(devn) = BASS_MusicLoad(BASSFALSE, StrPtr(cmd.filename), 0, 0, BASS_MUSIC_RAMPS Or BASS_SAMPLE_LOOP Or BASS_SAMPLE_FLOAT, 1)
        cmdOpen(devn).Caption = "Open file..."
        Call Error_("Can't play the file")
        Exit Sub
    End If
    cmdOpen(devn).Caption = cmd.FileTitle
    Call BASS_ChannelPlay(chan(devn), BASSFALSE) ' play new stream
End Sub

Private Sub cmbDevice_Change(devn As Integer)
    ' device selection changed
    Dim sel As Long
    sel = cmbDevice(devn).ListIndex
    If outdev(devn) = sel Then Exit Sub
    If BASS_Init(sel, 44100, 0, hWnd, 0) = BASSFALSE Then ' initialize new device
        Call Error_("Can't initialize device")
        cmbDevice(devn).ListIndex = 0
        Exit Sub
    End If
    If (chan(devn)) Then Call BASS_ChannelSetDevice(chan(devn), sel) ' move channel to new device
    Call BASS_SetDevice(outdev(devn)) ' set context to old device
    Call BASS_Free ' free it
    outdev(devn) = sel
End Sub

Private Sub cmdSwap_Click() ' swap channel devices
    ' swap handles
    Dim temp As Long
    temp = chan(0)
    chan(0) = chan(1)
    chan(1) = temp

    ' swap text
    Dim temp1 As String
    temp1 = cmdOpen(0).Caption
    cmdOpen(0).Caption = cmdOpen(1).Caption
    cmdOpen(1).Caption = temp1

    ' update the channel devices
    Call BASS_ChannelSetDevice(chan(0), outdev(0))
    Call BASS_ChannelSetDevice(chan(1), outdev(1))
End Sub

Private Sub cmdClone_Click(devn As Integer) ' clone on device #1 / #2
    Dim di As BASS_CHANNELINFO
    If (BASS_ChannelGetInfo(chan(devn Xor 1), di) = 0) Then
        Call Error_("Nothing to clone")
        Exit Sub
    End If

    Call BASS_StreamFree(chan(devn)) ' free old stream
    Call BASS_SetDevice(outdev(devn)) ' set the device to create stream on
    chan(devn) = BASS_StreamCreate(di.freq, di.chans, di.flags, STREAMPROC_PUSH, 0)
    If (chan(devn) = 0) Then  ' create a "push" stream
        cmdOpen(devn).Caption = "Open file..."
        Call Error_("Can't create clone")
    Else
        Dim info As BASS_INFO
        Call BASS_GetInfo(info) ' get latency info
        Call BASS_ChannelLock(chan(devn Xor 1), BASSTRUE) ' lock source stream to synchonise buffer contents
        Call BASS_ChannelSetDSP(chan(devn Xor 1), AddressOf CloneDSP, chan(devn), 0) ' set DSP to feed data to clone
        ' copy buffered data to clone
        Dim d As Long
        d = BASS_ChannelSeconds2Bytes(chan(devn), info.latency / 1000#)  ' playback delay
        Dim c As Long
        c = BASS_ChannelGetData(chan(devn Xor 1), 0, BASS_DATA_AVAILABLE)
        Dim buf() As Byte
        ReDim buf(c - 1)
        c = BASS_ChannelGetData(chan(devn Xor 1), buf(0), c)
        If (c > d) Then Call BASS_StreamPutData(chan(devn), buf(d), c - d)
        Erase buf
        Call BASS_ChannelLock(chan(devn Xor 1), BASSFALSE) ' unlock source stream
        Call BASS_ChannelPlay(chan(devn), BASSFALSE) ' play clone
        cmdOpen(devn).Caption = "clone"
    End If
End Sub

