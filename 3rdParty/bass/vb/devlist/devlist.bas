Attribute VB_Name = "devlist"
'    BASS device list example
'    Copyright (c) 2014-2022 Un4seen Developments Ltd.

Private Declare Function GetVersion Lib "kernel32" () As Long

Dim Msg As String

Private Sub DisplayDeviceInfo(di As BASS_DEVICEINFO)
    Dim driver As String
    Dim Path As String
    Dim name As String
    
    driver = VBStrFromAnsiPtr(di.driver)
    name = VBStrFromAnsiPtr(di.name)
#If 0 Then ' #ifdef _WIN32
    Path = VBStrFromAnsiPtr(di.driver + Len(name) + 1)
    If Path <> "" Then
        Msg = Msg & name & vbCrLf & vbTab & "driver: " & driver & vbCrLf & vbTab & "type: " & vbCrLf & vbTab & Path
    End If
#End If
    Msg = Msg & name & vbCrLf & vbTab & "driver: " & driver & vbCrLf & vbTab & "type: "
    Select Case (di.flags And BASS_DEVICE_TYPE_MASK)
        Case BASS_DEVICE_TYPE_NETWORK
            Msg = Msg & "Remote Network"
        Case BASS_DEVICE_TYPE_SPEAKERS
            Msg = Msg & "Speakers"
        Case BASS_DEVICE_TYPE_LINE:
            Msg = Msg & "Line"
        Case BASS_DEVICE_TYPE_HEADPHONES:
            Msg = Msg & "Headphones"
        Case BASS_DEVICE_TYPE_MICROPHONE:
            Msg = Msg & "Microphone"
        Case BASS_DEVICE_TYPE_HEADSET:
            Msg = Msg & "Headset"
        Case BASS_DEVICE_TYPE_HANDSET:
            Msg = Msg & "Handset"
        Case BASS_DEVICE_TYPE_DIGITAL:
            Msg = Msg & "Digital"
        Case BASS_DEVICE_TYPE_SPDIF:
            Msg = Msg & "SPDIF"
        Case BASS_DEVICE_TYPE_HDMI:
            Msg = Msg & "HDMI"
        Case BASS_DEVICE_TYPE_DISPLAYPORT:
            Msg = Msg & "DisplayPort"
        Case Else
            Msg = Msg & "Unknown"
    End Select
    
    Msg = Msg & vbCrLf & vbTab & "flags:"
    If (di.flags And BASS_DEVICE_LOOPBACK) Then Msg = Msg & " loopback"
    If (di.flags And BASS_DEVICE_ENABLED) Then Msg = Msg & " enabled"
    If (di.flags And BASS_DEVICE_DEFAULT) Then Msg = Msg & " default"
    Msg = Msg & " (" & Hex(di.flags) & ")" & vbCrLf
End Sub

Sub Main()
    Dim di As BASS_DEVICEINFO
    Dim a As Integer
    Msg = Msg & "Output Devices" & vbCrLf
    a = 1
    Do While BASS_GetDeviceInfo(a, di)
        Msg = Msg & a & ": "
        Call DisplayDeviceInfo(di)
        a = a + 1
    Loop
    
    Msg = Msg & vbCrLf & "Input Devices" & vbCrLf
    a = 0
    Do While BASS_RecordGetDeviceInfo(a, di)
        Msg = Msg & a & ": "
        Call DisplayDeviceInfo(di)
        If (GetVersion() And &HFF) < 6 Then ' only list inputs before Windows Vista (they're in device list after)
            ' list inputs
            Dim b As Long
            Dim n As Long
            Call BASS_RecordInit(a)
            b = 0
            Do
                n = BASS_RecordGetInputName(b)
                If n = 0 Then Exit Do
                Msg = Msg & vbTab & "input " & b & ": " & VBStrFromAnsiPtr(n) & vbCrLf
                b = b + 1
            Loop
            Call BASS_RecordFree
        End If
        a = a + 1
    Loop

    'display in a MsgBox
    MsgBox Msg
End Sub

