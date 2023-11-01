Attribute VB_Name = "mNetradio"
'/*
'    BASS internet radio example
'    Copyright (c) 2002-2022 Un4seen Developments Ltd.
'
'   other code: frmNetradio.frm
'*/

Option Explicit

Private Declare Sub InitializeCriticalSection Lib "kernel32" (lpCriticalSection As CRITICAL_SECTION)
Private Declare Sub DeleteCriticalSection Lib "kernel32" (lpCriticalSection As CRITICAL_SECTION)
Private Declare Sub EnterCriticalSection Lib "kernel32" (lpCriticalSection As CRITICAL_SECTION)
Private Declare Sub LeaveCriticalSection Lib "kernel32" (lpCriticalSection As CRITICAL_SECTION)
Private Type CRITICAL_SECTION
    Reserved1 As Long
    Reserved2 As Long
    Reserved3 As Long
    Reserved4 As Long
    Reserved5 As Long
    Reserved6 As Long
End Type

Private Declare Function CreateThread Lib "kernel32" (lpThreadAttributes As Any, ByVal dwStackSize As Long, ByVal lpStartAddress As Long, ByVal lpParameter As Long, ByVal dwCreationFlags As Long, lpThreadID As Long) As Long
Private Declare Function CloseHandle Lib "kernel32" (ByVal hObject As Long) As Long

' HLS definitions (copied from BASSHLS.H)
Private Const BASS_SYNC_HLS_SEGMENT = &H10300
Private Const BASS_TAG_HLS_EXTINF = &H14000

Dim chan As Long ' stream handle
Dim lock_  As CRITICAL_SECTION
Dim req As Long ' request number/counter
Dim urls As Variant

' display error messages
Private Sub Error_(ByVal es As String)
    Call MsgBox(es & vbCrLf & "(error code: " & BASS_ErrorGetCode() & ")", vbExclamation, "Error")
End Sub

Private Function isIDEmode() As Boolean
    On Error Resume Next
    Debug.Print 1 / 0 ' this line won't make it to the compiled exe but will throw an error in the IDE
    If Err Then isIDEmode = True
End Function

' update stream title from metadata
Public Sub DoMeta()
    Dim meta As Long
    Dim metaTxt As String
    meta = BASS_ChannelGetTags(chan, BASS_TAG_META)
    If meta Then ' got Shoutcast metadata
        metaTxt = VBStrFromAnsiPtr(meta)
        Dim p As Long
        p = InStr(metaTxt, "StreamTitle='") ' locate the title
        If p Then
            Dim p2 As Long
            p2 = InStr(p, metaTxt, "';") ' locate the end of it
            If p2 Then
                frmNetradio.lbl30.Caption = Mid(metaTxt, p + 13, p2 - (p + 13))
            End If
        End If
    Else
        meta = BASS_ChannelGetTags(chan, BASS_TAG_OGG)
        Dim artist As String, title As String
        If meta Then ' got Icecast/OGG tags
            Do
                metaTxt = VBStrFromAnsiPtr(meta)
                If metaTxt = vbNullString Then Exit Do
                If Left(metaTxt, 7) = "artist=" Then ' found the artist
                    artist = Mid(metaTxt, 8)
                ElseIf Left(metaTxt, 6) = "title=" Then ' found the title
                    artist = Mid(metaTxt, 7)
                End If
                meta = meta + Len(metaTxt) + 1
            Loop
            If title <> "" Then
                If artist <> "" Then
                    frmNetradio.lbl30.Caption = artist & " - " & title
                Else
                    frmNetradio.lbl30.Caption = title
                End If
            End If
        Else
            meta = BASS_ChannelGetTags(chan, BASS_TAG_HLS_EXTINF)
            If meta Then ' got HLS segment info
                metaTxt = VBStrFromAnsiPtr(meta)
                p = InStr(meta, ",")
                If p Then frmNetradio.lbl30.Caption = Mid(metaTxt, p + 1)
            End If
        End If
    End If
End Sub

Public Sub OpenURL(ByVal index As Long)
    Dim c As Long, r As Long
    Call EnterCriticalSection(lock_) ' make sure only 1 thread at a time can do the following
    req = req + 1 ' increment the request counter for this request
    r = req
    Call LeaveCriticalSection(lock_)
    If chan Then BASS_StreamFree (chan) ' close old stream
    frmNetradio.lbl31.Caption = "connecting..."
    frmNetradio.lbl30.Caption = ""
    frmNetradio.lbl32.Caption = ""
    Dim url As String
    If index = 10 Then
        url = frmNetradio.txt20.Text ' custom url
    Else
        url = urls(index) ' preset
    End If
    c = BASS_StreamCreateURL(url, 0, BASS_STREAM_BLOCK Or BASS_STREAM_STATUS Or BASS_STREAM_AUTOFREE Or BASS_SAMPLE_FLOAT, AddressOf StatusProc, r) ' open URL
    Call EnterCriticalSection(lock_)
    If r <> req Then ' there is a newer request, discard this stream
        Call LeaveCriticalSection(lock_)
        If c Then Call BASS_StreamFree(c)
        Exit Sub
    End If
    chan = c ' this is now the current stream
    Call LeaveCriticalSection(lock_)
    If chan = 0 Then ' failed to open
        frmNetradio.lbl31.Caption = "not playing"
        Call Error_("Can't play the stream")
    Else
        ' only needed the DOWNLOADPROC to receive HTTP/ICY tags, so disable it now
        Dim proc As Long
        Call BASS_ChannelSetAttributeEx(chan, BASS_ATTRIB_DOWNLOADPROC, proc, LenB(proc))
        ' set syncs for stream title updates
        Call BASS_ChannelSetSync(chan, BASS_SYNC_META, 0, AddressOf MetaSync, 0) ' Shoutcast
        Call BASS_ChannelSetSync(chan, BASS_SYNC_OGG_CHANGE, 0, AddressOf MetaSync, 0) ' Icecast/OGG
        Call BASS_ChannelSetSync(chan, BASS_SYNC_HLS_SEGMENT, 0, AddressOf MetaSync, 0) ' HLS
        ' set sync for stalling/buffering
        Call BASS_ChannelSetSync(chan, BASS_SYNC_STALL, 0, AddressOf StallSync, 0)
        ' set sync for end of stream (when freed due to AUTOFREE)
        Call BASS_ChannelSetSync(chan, BASS_SYNC_FREE, 0, AddressOf FreeSync, 0)
        ' play it!
        Call BASS_ChannelPlay(chan, BASSFALSE)
        ' start buffer monitoring (and display stream info when done)
        frmNetradio.tmrStall.Enabled = True
    End If
End Sub

Public Sub Preset(index As Integer)
    ' preset
    If frmNetradio.chk41.value = vbChecked Then
        Call BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, vbNullString) ' disable proxy
    Else
        Call BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, StrPtr(frmNetradio.txt40.Text)) ' set proxy server
    End If
    Call CloseHandle(CreateThread(ByVal 0&, 0, AddressOf OpenURL, index, 0, ByVal 0&))
End Sub

Public Sub TimerProc()
    ' monitor buffering progress
    Dim active As Long
    active = BASS_ChannelIsActive(chan)
    If active = BASS_ACTIVE_STALLED Then
        frmNetradio.lbl31.Caption = "buffering... " & 100 - Int(BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFERING))
    Else
        frmNetradio.tmrStall.Enabled = False ' finished buffering, stop monitoring
        If active Then
            frmNetradio.lbl31.Caption = "playing"
            ' get the stream name and URL
            Dim icy As Long
            icy = BASS_ChannelGetTags(chan, BASS_TAG_ICY)
            If icy = 0 Then
                icy = BASS_ChannelGetTags(chan, BASS_TAG_HTTP) ' no ICY tags, try HTTP
                If icy Then
                    Dim icyTxt As String
                    Do
                        icyTxt = VBStrFromAnsiPtr(icy)
                        If icyTxt = "" Then Exit Do
                        If Left(icyTxt, 9) = "icy-name:" Then
                            frmNetradio.lbl31.Caption = Mid(icyTxt, 10)
                        ElseIf Left(icyTxt, 8) = "icy-url:" Then
                            frmNetradio.lbl31.Caption = Mid(icyTxt, 9)
                        End If
                        icy = icy + Len(icyTxt) + 1
                    Loop
                End If
            End If
            ' get the stream title
            Call DoMeta
        End If
    End If
End Sub

Public Sub Initialize()
    Call InitializeCriticalSection(lock_)
    
    ' change and set the current path, to prevent from VB not finding BASS.DLL
    ChDrive App.Path
    ChDir App.Path

    ' check the correct BASS was loaded
    On Error Resume Next ' bass.dll not found
    If (HiWord(BASS_GetVersion) <> BASSVERSION) Then
        Call MsgBox("An incorrect version of BASS.DLL was loaded", vbCritical)
        Unload frmNetradio
        Exit Sub
    End If
    On Error GoTo 0
    
    ' preset stream URLs
    urls = Array("http://stream-dc1.radioparadise.com/rp_192m.ogg", "http://www.radioparadise.com/m3u/mp3-32.m3u", _
        "http://somafm.com/secretagent.pls", "http://somafm.com/secretagent32.pls", _
        "http://somafm.com/suburbsofgoa.pls", "http://somafm.com/suburbsofgoa32.pls", _
        "http://bassdrive.com/bassdrive.m3u", "http://bassdrive.com/bassdrive3.m3u", _
        "http://sc6.radiocaroline.net:8040/listen.pls", "http://sc2.radiocaroline.net:8010/listen.pls")
       
    Call BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1) ' enable playlist processing
    
    ' initialize default output device
    If BASS_Init(-1, 44100, 0, frmNetradio.hWnd, 0) = BASSFALSE Then
        Call Error_("Can't initialize device")
        Unload frmNetradio
        Exit Sub
    End If
            
    Call BASS_PluginLoad("bass_aac", 0) ' load BASS_AAC (if present) for AAC support on older Windows
    Call BASS_PluginLoad("bassflac", 0) ' load BASSFLAC (if present) for FLAC support
    Call BASS_PluginLoad("bassopus", 0) ' load BASSOPUS (if present) for OPUS support
    Call BASS_PluginLoad("basshls", 0)  ' load BASSHLS (if present) for HLS support

    frmNetradio.txt20.Text = "http://"
    frmNetradio.tmrStall.Enabled = False
    frmNetradio.tmrStall.Interval = 50
End Sub

Public Sub Finish()
    On Error Resume Next
    Call BASS_Free
    Call BASS_PluginFree(0)
    Call DeleteCriticalSection(lock_)
End Sub

Public Sub MetaSync(ByVal handle As Long, ByVal channel As Long, ByVal data As Long, ByVal user As Long)
    Call DoMeta
End Sub

Public Sub StallSync(ByVal handle As Long, ByVal channel As Long, ByVal data As Long, ByVal user As Long)
    If data = 0 Then ' stalled
        frmNetradio.tmrStall.Enabled = True ' start buffer monitoring
    End If
End Sub

Public Sub FreeSync(ByVal handle As Long, ByVal channel As Long, ByVal data As Long, ByVal user As Long)
    chan = 0
    frmNetradio.lbl31.Caption = "not playing"
    frmNetradio.lbl30.Caption = ""
    frmNetradio.lbl32.Caption = ""
End Sub

Public Sub StatusProc(ByVal buffer As Long, ByVal length As Long, ByVal user As Long)
    If (buffer <> 0) And (length = 0) And (user = req) Then ' got HTTP/ICY tags, and this is still the current request
        frmNetradio.lbl32.Caption = VBStrFromAnsiPtr(buffer) ' display status
    End If
End Sub
