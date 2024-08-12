Attribute VB_Name = "BASSOPUS"
' BASSOPUS 2.4 Visual Basic module
' Copyright (c) 2012-2024 Un4seen Developments Ltd.
'
' See the BASSOPUS.CHM file for more detailed documentation

' BASS_CHANNELINFO type
Global Const BASS_CTYPE_STREAM_OPUS = &H11200

' Additional attributes
Global Const BASS_ATTRIB_OPUS_ORIGFREQ = &H13000
Global Const BASS_ATTRIB_OPUS_GAIN = &H13001

Type BASS_OPUS_HEAD
	version As Byte
	channels As Byte
	preskip As Integer
	inputrate As Long
	gain As Integer
	mapping As Byte
	streams As Byte
	coupled As Byte
	chanmap As Byte * 255
End Type

Global Const BASS_STREAMPROC_OPUS_LOSS = &H40000000

Declare Function BASS_OPUS_StreamCreate Lib "bassopus.dll" (ByRef head As BASS_OPUS_HEAD, ByVal flags As Long, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_OPUS_StreamCreateFile64 Lib "bassopus.dll" Alias "BASS_OPUS_StreamCreateFile" (ByVal mem As Long, ByVal file As Any, ByVal offset As Long, ByVal offsethigh As Long, ByVal length As Long, ByVal lengthhigh As Long, ByVal flags As Long) As Long
Declare Function BASS_OPUS_StreamCreateURL Lib "bassopus.dll" (ByVal url As String, ByVal offset As Long, ByVal flags As Long, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_OPUS_StreamCreateFileUser Lib "bassopus.dll" (ByVal system As Long, ByVal flags As Long, ByVal procs As Long, ByVal user As Long) As Long
Declare Function BASS_OPUS_StreamPutData Lib "bassopus.dll" (ByVal handle As Long, ByRef buffer As Any, ByVal length As Long) As Long

' 32-bit wrapper for 64-bit BASS function
Function BASS_OPUS_StreamCreateFile(ByVal mem As Long, ByVal file As Long, ByVal offset As Long, ByVal length As Long, ByVal flags As Long) As Long
BASS_OPUS_StreamCreateFile = BASS_OPUS_StreamCreateFile64(mem, file, offset, 0, length, 0, flags Or BASS_UNICODE)
End Function
