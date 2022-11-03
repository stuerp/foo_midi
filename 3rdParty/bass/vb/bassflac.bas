Attribute VB_Name = "BASSFLAC"
' BASSFLAC 2.4 Visual Basic module
' Copyright (c) 2004-2017 Un4seen Developments Ltd.
'
' See the BASSFLAC.CHM file for more detailed documentation

' BASS_CHANNELINFO type
Global Const BASS_CHANNEL_STREAM_FLAC = &H10900
Global Const BASS_CHANNEL_STREAM_FLAC_OGG = &H10901

' Additional tag types
Global Const BASS_TAG_FLAC_CUE = 12 ' cuesheet : TAG_FLAC_CUE structure
Global Const BASS_TAG_FLAC_PICTURE = &H12000 ' + index #, picture : TAG_FLAC_PICTURE structure
Global Const BASS_TAG_FLAC_METADATA = &H12400 ' + index #, application metadata : TAG_FLAC_METADATA structure

Type TAG_FLAC_PICTURE
	apic As Long        ' ID3v2 "APIC" picture type
	mime As Long        ' mime type
	desc As Long        ' description
	width As Long
	height As Long
	depth As Long
	colors As Long
	length As Long      ' data length
	data As Long
End Type

Type TAG_FLAC_CUE_TRACK_INDEX
	offset As Long      ' index offset relative to track offset (samples)
	offsethi As Long
	number As Long      ' index number
End Type

Type TAG_FLAC_CUE_TRACK
	offset As Long      ' track offset (samples)
	offsethi As Long
	number As Long      ' track number
	isrc As Long        ' ISRC
	flags As Long
	nindexes As Long    ' number of indexes
	indexes As Long     ' pointer to the indexes
End Type

Type TAG_FLAC_CUE
	catalog As Long     ' media catalog number
	leadin As Long      ' lead-in (samples)
	iscd As Long        ' a CD?
	ntracks As Long     ' number of tracks
	tracks As Long      ' pointer to the tracks
End Type

' TAG_FLAC_CUE_TRACK flags
Global Const TAG_FLAC_CUE_TRACK_DATA    1 ' data track
Global Const TAG_FLAC_CUE_TRACK_PRE     2 ' pre-emphasis

Type TAG_FLAC_METADATA
    id As String * 4
	length As Long      ' data length
	data As Long
End Type

Declare Function BASS_FLAC_StreamCreateFile64 Lib "bassflac.dll" Alias "BASS_FLAC_StreamCreateFile" (ByVal mem As Long, ByVal file As Any, ByVal offset As Long, ByVal offsethigh As Long, ByVal length As Long, ByVal lengthhigh As Long, ByVal flags As Long) As Long
Declare Function BASS_FLAC_StreamCreateURL Lib "bassflac.dll" (ByVal url As String, ByVal offset As Long, ByVal flags As Long, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_FLAC_StreamCreateFileUser Lib "bassflac.dll" (ByVal system As Long, ByVal flags As Long, ByVal procs As Long, ByVal user As Long) As Long

' 32-bit wrappers for 64-bit BASS functions
Function BASS_FLAC_StreamCreateFile(ByVal mem As Long, ByVal file As Long, ByVal offset As Long, ByVal length As Long, ByVal flags As Long) As Long
BASS_FLAC_StreamCreateFile = BASS_FLAC_StreamCreateFile64(mem, file, offset, 0, length, 0, flags Or BASS_UNICODE)
End Function

