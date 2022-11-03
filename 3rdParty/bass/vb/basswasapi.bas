Attribute VB_Name = "BASSWASAPI"
' BASSWASAPI 2.4 Visual Basic module
' Copyright (c) 2009-2015 Un4seen Developments Ltd.
'
' See the BASSWASAPI.CHM file for more detailed documentation

' Additional error codes returned by BASS_ErrorGetCode
Global Const BASS_ERROR_WASAPI = 5000    ' no WASAPI
Global Const BASS_ERROR_WASAPI_BUFFER = 5001 ' buffer is too large

' Device info structure
Type BASS_WASAPI_DEVICEINFO
    name As Long
    id As Long
    type As Long
    flags As Long
    minperiod As Single
    defperiod As Single
    mixfreq As Long
    mixchans As Long
End Type

Type BASS_WASAPI_INFO
    initflags As Long
    freq As Long
    chans As Long
    format As Long
    buflen As Long
    volmax As Single
    volmin As Single
    volstep As Single
End Type

' BASS_WASAPI_DEVICEINFO "type"
Global Const BASS_WASAPI_TYPE_NETWORKDEVICE = 0
Global Const BASS_WASAPI_TYPE_SPEAKERS = 1
Global Const BASS_WASAPI_TYPE_LINELEVEL = 2
Global Const BASS_WASAPI_TYPE_HEADPHONES = 3
Global Const BASS_WASAPI_TYPE_MICROPHONE = 4
Global Const BASS_WASAPI_TYPE_HEADSET = 5
Global Const BASS_WASAPI_TYPE_HANDSET = 6
Global Const BASS_WASAPI_TYPE_DIGITAL = 7
Global Const BASS_WASAPI_TYPE_SPDIF = 8
Global Const BASS_WASAPI_TYPE_HDMI = 9
Global Const BASS_WASAPI_TYPE_UNKNOWN = 10

' BASS_WASAPI_DEVICEINFO flags
#If 0 Then ' already defined in BASS.BAS
Global Const BASS_DEVICE_ENABLED = 1
Global Const BASS_DEVICE_DEFAULT = 2
Global Const BASS_DEVICE_INIT = 4
#End If
Global Const BASS_DEVICE_LOOPBACK = 8
Global Const BASS_DEVICE_INPUT = 16
Global Const BASS_DEVICE_UNPLUGGED = 32
Global Const BASS_DEVICE_DISABLED = 64

' BASS_WASAPI_Init flags
Global Const BASS_WASAPI_EXCLUSIVE = 1
Global Const BASS_WASAPI_AUTOFORMAT = 2
Global Const BASS_WASAPI_BUFFER = 4
Global Const BASS_WASAPI_EVENT = 16

' BASS_WASAPI_INFO "format"
Global Const BASS_WASAPI_FORMAT_FLOAT = 0
Global Const BASS_WASAPI_FORMAT_8BIT = 1
Global Const BASS_WASAPI_FORMAT_16BIT = 2
Global Const BASS_WASAPI_FORMAT_24BIT = 3
Global Const BASS_WASAPI_FORMAT_32BIT = 4

' BASS_WASAPI_Set/GetVolume modes
Global Const BASS_WASAPI_CURVE_DB = 0
Global Const BASS_WASAPI_CURVE_LINEAR = 1
Global Const BASS_WASAPI_CURVE_WINDOWS = 2
Global Const BASS_WASAPI_VOL_SESSION = 8

' Device notifications
Global Const BASS_WASAPI_NOTIFY_ENABLED = 0
Global Const BASS_WASAPI_NOTIFY_DISABLED = 1
Global Const BASS_WASAPI_NOTIFY_DEFOUTPUT = 2
Global Const BASS_WASAPI_NOTIFY_DEFINPUT = 3


Declare Function BASS_WASAPI_GetVersion Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_SetNotify Lib "basswasapi.dll" (ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_WASAPI_GetDeviceInfo Lib "basswasapi.dll" (ByVal device As Long, ByRef info As BASS_WASAPI_DEVICEINFO) As Long
Declare Function BASS_WASAPI_GetDeviceLevel Lib "basswasapi.dll" (ByVal device As Long, ByVal chan As Long) As Single
Declare Function BASS_WASAPI_SetDevice Lib "basswasapi.dll" (ByVal device As Long) As Long
Declare Function BASS_WASAPI_GetDevice Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_CheckFormat Lib "basswasapi.dll" (ByVal device As Long, ByVal freq As Long, ByVal chans As Long, ByVal flags As Long) As Long
Declare Function BASS_WASAPI_Init Lib "basswasapi.dll" (ByVal device As Long, ByVal freq As Long, ByVal chans As Long, ByVal flags As Long, ByVal buffer As Single, ByVal period As Single, ByVal proc As Long, ByVal user As Long) As Long
Declare Function BASS_WASAPI_Free Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_GetInfo Lib "basswasapi.dll" (ByRef info As BASS_WASAPI_INFO) As Long
Declare Function BASS_WASAPI_GetCPU Lib "basswasapi.dll" () As Single
Declare Function BASS_WASAPI_Lock Lib "basswasapi.dll" (ByVal lock_ As Long) As Long
Declare Function BASS_WASAPI_Start Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_Stop Lib "basswasapi.dll" (ByVal reset As Long) As Long
Declare Function BASS_WASAPI_IsStarted Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_SetVolume Lib "basswasapi.dll" (ByVal mode As Long, ByVal volume As Single) As Long
Declare Function BASS_WASAPI_GetVolume Lib "basswasapi.dll" (ByVal mode As Long) As Single
Declare Function BASS_WASAPI_SetMute Lib "basswasapi.dll" (ByVal mode As Long, ByVal mute As Long) As Long
Declare Function BASS_WASAPI_GetMute Lib "basswasapi.dll" (ByVal mode As Long) As Long
Declare Function BASS_WASAPI_PutData Lib "basswasapi.dll" (ByRef buffer As Any, ByVal length As Long) As Long
Declare Function BASS_WASAPI_GetData Lib "basswasapi.dll" (ByRef buffer As Any, ByVal length As Long) As Long
Declare Function BASS_WASAPI_GetLevel Lib "basswasapi.dll" () As Long
Declare Function BASS_WASAPI_GetLevelEx Lib "basswasapi.dll" (ByRef levels As Single, ByVal length As Single, ByVal flags As Long) As Long


Function WASAPIPROC(ByVal buffer As Long, ByVal length As Long, ByVal user As Long) As Long
    
    'CALLBACK FUNCTION !!!
    
    ' WASAPI callback function
    ' buffer : Buffer containing the sample data
    ' length : Number of bytes
    ' user   : The 'user' parameter given when calling BASS_WASAPI_Init
    ' RETURN : The number of bytes written (output devices), 0/1 = stop/continue (input devices)
    
End Function

Sub WASAPINOTIFYPROC(ByVal notify As Long, ByVal device As Long, ByVal user As Long)
    
    'CALLBACK FUNCTION !!!
    
    ' WASAPI device notification callback function.
    ' notify : The notification (BASS_WASAPI_NOTIFY_xxx)
    ' device : Device that the notification applies to
    ' user   : The 'user' parameter given when calling BASS_WASAPI_SetNotify
    
End Sub
