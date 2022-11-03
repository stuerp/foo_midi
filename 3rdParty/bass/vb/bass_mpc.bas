Attribute VB_Name = "bass_mpc"
Option Explicit

' Additional tags available from BASS_StreamGetTags
Global Const BASS_TAG_APE = 6	' APE tags

' BASS_CHANNELINFO type
Global Const BASS_CTYPE_STREAM_MPC = &H10A00

Declare Function BASS_MPC_StreamCreateFile64 Lib "bass_mpc.dll" Alias "BASS_MPC_StreamCreateFile" (ByVal mem As Long, ByVal file As Any, ByVal offset As Long, ByVal offsethigh As Long, ByVal length As Long, ByVal lengthhigh As Long, ByVal flags As Long) As Long
Declare Function BASS_MPC_StreamCreateURL Lib "bass_mpc.dll" (ByVal url As String, ByVal offset As Long, ByVal flags As Long, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_MPC_StreamCreateFileUser Lib "bass_mpc.dll" (ByVal system As Long, ByVal flags As Long, ByVal procs As Long, ByVal user As Long) As Long

' 32-bit wrappers for 64-bit BASS functions
Function BASS_MPC_StreamCreateFile(ByVal mem As Long, ByVal file As Long, ByVal offset As Long, ByVal length As Long, ByVal flags As Long) As Long
BASS_MPC_StreamCreateFile = BASS_MPC_StreamCreateFile64(mem, file, offset, 0, length, 0, flags Or BASS_UNICODE)
End Function
