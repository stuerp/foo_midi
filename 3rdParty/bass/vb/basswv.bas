Attribute VB_Name = "BASSWV"
' BASSWV 2.4 Visual Basic module
' Copyright (c) 2007-2020 Un4seen Developments Ltd.
'
' See the BASSWV.CHM file for more detailed documentation

' BASS_CHANNELINFO type
Global Const BASS_CTYPE_STREAM_WV = &H10500

Declare Function BASS_WV_StreamCreateFile64 Lib "basswv.dll" Alias "BASS_WV_StreamCreateFile" (ByVal mem As Long, ByVal file As Any, ByVal offset As Long, ByVal offsethigh As Long, ByVal length As Long, ByVal lengthhigh As Long, ByVal flags As Long) As Long
Declare Function BASS_WV_StreamCreateURL Lib "basswv.dll" (ByVal url As String, ByVal offset As Long, ByVal flags As Long, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_WV_StreamCreateFileUser Lib "basswv.dll" (ByVal system As Long, ByVal flags As Long, ByVal procs As Long, ByVal user As Long) As Long
Declare Function BASS_WV_StreamCreateFileUserEx Lib "basswv.dll" (ByVal system As Long, ByVal flags As Long, ByVal procs As Long, ByVal user As Long, ByVal userwvc As Long) As Long

' 32-bit wrapper for 64-bit BASS function
Function BASS_WV_StreamCreateFile(ByVal mem As Long, ByVal file As Long, ByVal offset As Long, ByVal length As Long, ByVal flags As Long) As Long
BASS_WV_StreamCreateFile = BASS_WV_StreamCreateFile64(mem, file, offset, 0, length, 0, flags Or BASS_UNICODE)
End Function
