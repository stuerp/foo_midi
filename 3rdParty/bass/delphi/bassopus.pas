{
  BASSOPUS 2.4 Delphi unit
  Copyright (c) 2012-2015 Un4seen Developments Ltd.

  See the BASSOPUS.CHM file for more detailed documentation
}

unit BASSOPUS;

interface

{$IFDEF MSWINDOWS}
uses BASS, Windows;
{$ELSE}
uses BASS;
{$ENDIF}

const
  // BASS_CHANNELINFO type
  BASS_CTYPE_STREAM_OPUS        = $11200;

  // Additional attributes
  BASS_ATTRIB_OPUS_ORIGFREQ     = $13000;
  BASS_ATTRIB_OPUS_GAIN         = $13001;

const
{$IFDEF MSWINDOWS}
  bassopusdll = 'bassopus.dll';
{$ENDIF}
{$IFDEF LINUX}
  bassopusdll = 'libbassopus.so';
{$ENDIF}
{$IFDEF MACOS}
  bassopusdll = 'libbassopus.dylib';
{$ENDIF}

function BASS_OPUS_StreamCreateFile(mem:BOOL; fl:pointer; offset,length:QWORD; flags:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassopusdll;
function BASS_OPUS_StreamCreateURL(url:PAnsiChar; offset,flags:DWORD; proc:DOWNLOADPROC; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassopusdll;
function BASS_OPUS_StreamCreateFileUser(system,flags:DWORD; var procs:BASS_FILEPROCS; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassopusdll;

implementation

end.