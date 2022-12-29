{
  BASSWV 2.4 Delphi unit
  Copyright (c) 2007-2020 Un4seen Developments Ltd.

  See the BASSWV.CHM file for more detailed documentation
}

unit BASSWV;

interface

{$IFDEF MSWINDOWS}
uses BASS, Windows;
{$ELSE}
uses BASS;
{$ENDIF}

const
  // BASS_CHANNELINFO type
  BASS_CTYPE_STREAM_WV        = $10500;

const
{$IFDEF MSWINDOWS}
  basswvdll = 'basswv.dll';
{$ENDIF}
{$IFDEF LINUX}
  basswvdll = 'libbasswv.so';
{$ENDIF}
{$IFDEF ANDROID}
  basswvdll = 'libbasswv.so';
{$ENDIF}
{$IFDEF MACOS}
  {$IFDEF IOS}
    basswvdll = 'libbasswv.a';
  {$ELSE}
    basswvdll = 'libbasswv.dylib';
  {$ENDIF}
{$ENDIF}

function BASS_WV_StreamCreateFile(mem:BOOL; f:pointer; offset,length:QWORD; flags:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external basswvdll;
function BASS_WV_StreamCreateURL(url:PChar; offset:DWORD; flags:DWORD; proc:DOWNLOADPROC; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external basswvdll;
function BASS_WV_StreamCreateFileUser(system,flags:DWORD; var procs:BASS_FILEPROCS; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external basswvdll;
function BASS_WV_StreamCreateFileUserEx(system,flags:DWORD; var procs:BASS_FILEPROCS; user,userwvc:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external basswvdll;

implementation

end.