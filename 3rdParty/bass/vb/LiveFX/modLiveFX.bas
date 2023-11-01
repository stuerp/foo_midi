Attribute VB_Name = "modLiveFX"
'///////////////////////////////////////////////////////////////////////////////
' modLiveFX.bas - Copyright (c) 2002-2007 (: JOBnik! :) [Arthur Aminov, ISRAEL]
'                                                       [http://www.jobnik.org]
'                                                       [  jobnik@jobnik.org  ]
'
' Other source: frmLiveFX.frm
'
' BASS full-duplex recording test with effects
' Originally translated from - livefx.c - Example of Ian Luck
'///////////////////////////////////////////////////////////////////////////////

Option Explicit

Public fxtype(3) As Long ' { BASS_FX_DX8_REVERB, BASS_FX_DX8_GARGLE, BASS_FX_DX8_FLANGER, BASS_FX_DX8_CHORUS }
Public info As BASS_INFO
Public rchan As Long    ' recording channel
Public chan As Long     ' playback stream
Public fx(3) As Long    ' FX handles
Public input_ As Long   ' current input source
Public volume As Single ' volume level

Public Const DEFAULTRATE = 44100
Public Const TARGETBUFS = 2 ' managed buffer level target (higher = more safety margin + higher latency)

Public prebuf As Boolean   ' prebuffering
Public initrate As Long    ' initial output rate
Public rate As Long        ' current output rate
Public ratedelay As Long   ' rate change delay
Public buftarget As Long   ' target buffer amount
Public buflevel As Single  ' current/average buffer amount

' Display error message
Public Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & vbCrLf & "error code: " & BASS_ErrorGetCode, vbExclamation, "Error")
End Sub

' STREAMPROC that pulls data from the recording
Public Function MyStreamProc(ByVal handle As Long, ByVal buffer As Long, ByVal length As Long, ByVal user As Long) As Long
    Dim got As Long
    got = BASS_ChannelGetData(rchan, 0, BASS_DATA_AVAILABLE) ' get recording buffer level
    buflevel = (buflevel * 49 + got) / 50#
    If got = 0 Then prebuf = True ' prebuffer again if buffer is empty
    If prebuf Then ' prebuffering
        buflevel = got
        If got < IIf(buftarget, buftarget, length) Then Exit Function ' haven't got enough yet
        prebuf = False
        ratedelay = 10
    End If
    
    Dim r As Long
    r = rate
    If buftarget Then
#If 1 Then ' target buffer amount = minimum
        If got < buftarget Then ' buffer level is low
            r = initrate * 0.999 ' slow down slightly
            ratedelay = 10 + (buftarget - got) / 16 ' prevent speeding-up again too soon
        ElseIf ratedelay = 0 Then
            If buflevel >= buftarget * 1.1 Then ' buffer level is high
                r = initrate * 1.001 ' speed up slightly
            Else ' buffer level is in range
                r = initrate ' normal speed
            End If
        Else
            ratedelay = ratedelay - 1
#Else ' target buffer amount = average
        If buflevel < buftarget Then ' buffer level is low
            r = initrate * 0.999 ' slow down slightly
        ElseIf buflevel >= buftarget * 1.1 Then ' buffer level is high
            r = initrate * 1.001 ' speed up slightly
        Else ' buffer level is in range
            r = initrate ' normal speed
#End If
        End If
    Else
        r = initrate ' normal speed
        If r <> rate Then
            rate = r
            Call BASS_ChannelSetAttribute(chan, BASS_ATTRIB_FREQ, rate)
        End If
    End If
    MyStreamProc = BASS_ChannelGetData(rchan, ByVal buffer, length) ' get the data
End Function
 
