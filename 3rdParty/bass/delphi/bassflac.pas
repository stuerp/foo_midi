{
  BASSFLAC 2.4 Delphi unit
  Copyright (c) 2004-2017 Un4seen Developments Ltd.

  See the BASSFLAC.CHM file for more detailed documentation
}

unit BassFLAC;

interface

{$IFDEF MSWINDOWS}
uses BASS, Windows;
{$ELSE}
uses BASS;
{$ENDIF}

const
  // BASS_CHANNELINFO type
  BASS_CTYPE_STREAM_FLAC        = $10900;
  BASS_CTYPE_STREAM_FLAC_OGG    = $10901;

  // Additional tag type
  BASS_TAG_FLAC_CUE             = 12; // cuesheet : TAG_FLAC_CUE structure
  BASS_TAG_FLAC_PICTURE         = $12000; // + index #, picture : TAG_FLAC_PICTURE structure
  BASS_TAG_FLAC_METADATA        = $12400; // + index #, application metadata : TAG_FLAC_METADATA structure

type
  TAG_FLAC_PICTURE = record
    apic: DWORD;		// ID3v2 "APIC" picture type
    mime: PAnsiChar;	// mime type
    desc: PAnsiChar;	// description
    width: DWORD;
    height: DWORD;
    depth: DWORD;
    colors: DWORD;
    length: DWORD;		// data length
    data: Pointer;
  end;

  TAG_FLAC_METADATA = record
    id: Array[0..3] of AnsiChar;
	length: DWORD;		// data length
    data: Pointer;
  end;

const
{$IFDEF MSWINDOWS}
  bassflacdll = 'bassflac.dll';
{$ENDIF}
{$IFDEF LINUX}
  bassflacdll = 'libbassflac.so';
{$ENDIF}
{$IFDEF ANDROID}
  bassflacdll = 'libbassflac.so';
{$ENDIF}
{$IFDEF MACOS}
  {$IFDEF IOS}
    bassflacdll = 'bassflac.framework/bassflac';
  {$ELSE}
    bassflacdll = 'libbassflac.dylib';
  {$ENDIF}
{$ENDIF}

function BASS_FLAC_StreamCreateFile(mem:BOOL; f:Pointer; offset,length:QWORD; flags:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassflacdll;
function BASS_FLAC_StreamCreateURL(url:PChar; offset:DWORD; flags:DWORD; proc:DOWNLOADPROC; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassflacdll;
function BASS_FLAC_StreamCreateFileUser(system,flags:DWORD; var procs:BASS_FILEPROCS; user:Pointer): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassflacdll;

implementation

end.
