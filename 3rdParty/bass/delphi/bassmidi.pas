{
  BASSMIDI 2.4 Delphi unit
  Copyright (c) 2006-2022 Un4seen Developments Ltd.

  See the BASSMIDI.CHM file for more detailed documentation
}

unit BASSMIDI;

interface

{$IFDEF MSWINDOWS}
uses BASS, Windows;
{$ELSE}
uses BASS;
{$ENDIF}

const
  // Additional error codes returned by BASS_ErrorGetCode
  BASS_ERROR_MIDI_INCLUDE    = 7000; // SFZ include file could not be opened

  // Additional config options
  BASS_CONFIG_MIDI_COMPACT   = $10400;
  BASS_CONFIG_MIDI_VOICES    = $10401;
  BASS_CONFIG_MIDI_AUTOFONT  = $10402;
  BASS_CONFIG_MIDI_IN_PORTS  = $10404;
  BASS_CONFIG_MIDI_SAMPLETHREADS = $10406;
  BASS_CONFIG_MIDI_SAMPLEMEM = $10407;
  BASS_CONFIG_MIDI_SAMPLEREAD = $10408;
  BASS_CONFIG_MIDI_SAMPLELOADING = $1040a;

  // Additional BASS_SetConfigPtr options
  BASS_CONFIG_MIDI_DEFFONT   = $10403;
  BASS_CONFIG_MIDI_SFZHEAD   = $10409;

  // Additional sync types
  BASS_SYNC_MIDI_MARK        = $10000;
  BASS_SYNC_MIDI_MARKER      = $10000;
  BASS_SYNC_MIDI_CUE         = $10001;
  BASS_SYNC_MIDI_LYRIC       = $10002;
  BASS_SYNC_MIDI_TEXT        = $10003;
  BASS_SYNC_MIDI_EVENT       = $10004;
  BASS_SYNC_MIDI_TICK        = $10005;
  BASS_SYNC_MIDI_TIMESIG     = $10006;
  BASS_SYNC_MIDI_KEYSIG      = $10007;

  // Additional BASS_MIDI_StreamCreateFile/etc flags
  BASS_MIDI_NODRUMPARAM      = $400;
  BASS_MIDI_NOSYSRESET       = $800;
  BASS_MIDI_DECAYEND         = $1000;
  BASS_MIDI_NOFX             = $2000;
  BASS_MIDI_DECAYSEEK        = $4000;
  BASS_MIDI_NOCROP           = $8000;
  BASS_MIDI_NOTEOFF1         = $10000;
  BASS_MIDI_ASYNC            = $400000;
  BASS_MIDI_SINCINTER        = $800000;

  // BASS_MIDI_FontInit flags
  BASS_MIDI_FONT_MEM         = $10000;
  BASS_MIDI_FONT_MMAP        = $20000;
  BASS_MIDI_FONT_XGDRUMS     = $40000;
  BASS_MIDI_FONT_NOFX        = $80000;
  BASS_MIDI_FONT_LINATTMOD   = $100000;
  BASS_MIDI_FONT_LINDECVOL   = $200000;
  BASS_MIDI_FONT_NORAMPIN    = $400000;
  BASS_MIDI_FONT_NOLIMITS    = $800000;
  BASS_MIDI_FONT_MINFX       = $1000000;

  // BASS_MIDI_StreamSet/GetFonts flag
  BASS_MIDI_FONT_EX          = $1000000; // BASS_MIDI_FONTEX
  BASS_MIDI_FONT_EX2         = $2000000; // BASS_MIDI_FONTEX2

  // Marker types
  BASS_MIDI_MARK_MARKER      = 0; // marker events
  BASS_MIDI_MARK_CUE         = 1; // cue events
  BASS_MIDI_MARK_LYRIC       = 2; // lyric events
  BASS_MIDI_MARK_TEXT        = 3; // text events
  BASS_MIDI_MARK_TIMESIG     = 4; // time signature
  BASS_MIDI_MARK_KEYSIG      = 5; // key signature
  BASS_MIDI_MARK_COPY        = 6; // copyright notice
  BASS_MIDI_MARK_TRACK       = 7; // track name
  BASS_MIDI_MARK_INST        = 8; // instrument name
  BASS_MIDI_MARK_TRACKSTART  = 9; // track start (SMF2)
  BASS_MIDI_MARK_TICK        = $10000; // flag: get position in ticks (otherwise bytes)

  // MIDI events
  MIDI_EVENT_NOTE            = 1;
  MIDI_EVENT_PROGRAM         = 2;
  MIDI_EVENT_CHANPRES        = 3;
  MIDI_EVENT_PITCH           = 4;
  MIDI_EVENT_PITCHRANGE      = 5;
  MIDI_EVENT_DRUMS           = 6;
  MIDI_EVENT_FINETUNE        = 7;
  MIDI_EVENT_COARSETUNE      = 8;
  MIDI_EVENT_MASTERVOL       = 9;
  MIDI_EVENT_BANK            = 10;
  MIDI_EVENT_MODULATION      = 11;
  MIDI_EVENT_VOLUME          = 12;
  MIDI_EVENT_PAN             = 13;
  MIDI_EVENT_EXPRESSION      = 14;
  MIDI_EVENT_SUSTAIN         = 15;
  MIDI_EVENT_SOUNDOFF        = 16;
  MIDI_EVENT_RESET           = 17;
  MIDI_EVENT_NOTESOFF        = 18;
  MIDI_EVENT_PORTAMENTO      = 19;
  MIDI_EVENT_PORTATIME       = 20;
  MIDI_EVENT_PORTANOTE       = 21;
  MIDI_EVENT_MODE            = 22;
  MIDI_EVENT_REVERB          = 23;
  MIDI_EVENT_CHORUS          = 24;
  MIDI_EVENT_CUTOFF          = 25;
  MIDI_EVENT_RESONANCE       = 26;
  MIDI_EVENT_RELEASE         = 27;
  MIDI_EVENT_ATTACK          = 28;
  MIDI_EVENT_DECAY           = 29;
  MIDI_EVENT_REVERB_MACRO    = 30;
  MIDI_EVENT_CHORUS_MACRO    = 31;
  MIDI_EVENT_REVERB_TIME     = 32;
  MIDI_EVENT_REVERB_DELAY    = 33;
  MIDI_EVENT_REVERB_LOCUTOFF = 34;
  MIDI_EVENT_REVERB_HICUTOFF = 35;
  MIDI_EVENT_REVERB_LEVEL    = 36;
  MIDI_EVENT_CHORUS_DELAY    = 37;
  MIDI_EVENT_CHORUS_DEPTH    = 38;
  MIDI_EVENT_CHORUS_RATE     = 39;
  MIDI_EVENT_CHORUS_FEEDBACK = 40;
  MIDI_EVENT_CHORUS_LEVEL    = 41;
  MIDI_EVENT_CHORUS_REVERB   = 42;
  MIDI_EVENT_USERFX          = 43;
  MIDI_EVENT_USERFX_LEVEL    = 44;
  MIDI_EVENT_USERFX_REVERB   = 45;
  MIDI_EVENT_USERFX_CHORUS   = 46;
  MIDI_EVENT_DRUM_FINETUNE   = 50;
  MIDI_EVENT_DRUM_COARSETUNE = 51;
  MIDI_EVENT_DRUM_PAN        = 52;
  MIDI_EVENT_DRUM_REVERB     = 53;
  MIDI_EVENT_DRUM_CHORUS     = 54;
  MIDI_EVENT_DRUM_CUTOFF     = 55;
  MIDI_EVENT_DRUM_RESONANCE  = 56;
  MIDI_EVENT_DRUM_LEVEL      = 57;
  MIDI_EVENT_DRUM_USERFX     = 58;
  MIDI_EVENT_SOFT            = 60;
  MIDI_EVENT_SYSTEM          = 61;
  MIDI_EVENT_TEMPO           = 62;
  MIDI_EVENT_SCALETUNING     = 63;
  MIDI_EVENT_CONTROL         = 64;
  MIDI_EVENT_CHANPRES_VIBRATO = 65;
  MIDI_EVENT_CHANPRES_PITCH  = 66;
  MIDI_EVENT_CHANPRES_FILTER = 67;
  MIDI_EVENT_CHANPRES_VOLUME = 68;
  MIDI_EVENT_MOD_VIBRATO     = 69;
  MIDI_EVENT_MODRANGE        = 69;
  MIDI_EVENT_BANK_LSB        = 70;
  MIDI_EVENT_KEYPRES         = 71;
  MIDI_EVENT_KEYPRES_VIBRATO = 72;
  MIDI_EVENT_KEYPRES_PITCH   = 73;
  MIDI_EVENT_KEYPRES_FILTER  = 74;
  MIDI_EVENT_KEYPRES_VOLUME  = 75;
  MIDI_EVENT_SOSTENUTO       = 76;
  MIDI_EVENT_MOD_PITCH       = 77;
  MIDI_EVENT_MOD_FILTER      = 78;
  MIDI_EVENT_MOD_VOLUME      = 79;
  MIDI_EVENT_VIBRATO_RATE    = 80;
  MIDI_EVENT_VIBRATO_DEPTH   = 81;
  MIDI_EVENT_VIBRATO_DELAY   = 82;
  MIDI_EVENT_MASTER_FINETUNE = 83;
  MIDI_EVENT_MASTER_COARSETUNE = 84;
  MIDI_EVENT_MIXLEVEL        = $10000;
  MIDI_EVENT_TRANSPOSE       = $10001;
  MIDI_EVENT_SYSTEMEX        = $10002;
  MIDI_EVENT_SPEED           = $10004;
  MIDI_EVENT_DEFDRUMS        = $10006;

  MIDI_EVENT_END             = 0;
  MIDI_EVENT_END_TRACK       = $10003;

  MIDI_EVENT_NOTES           = $20000;
  MIDI_EVENT_VOICES          = $20001;

  MIDI_SYSTEM_DEFAULT        = 0;
  MIDI_SYSTEM_GM1            = 1;
  MIDI_SYSTEM_GM2            = 2;
  MIDI_SYSTEM_XG             = 3;
  MIDI_SYSTEM_GS             = 4;

  // BASS_MIDI_StreamEvents modes
  BASS_MIDI_EVENTS_STRUCT    = 0; // BASS_MIDI_EVENT structures
  BASS_MIDI_EVENTS_RAW       = $10000; // raw MIDI event data
  BASS_MIDI_EVENTS_SYNC      = $1000000; // flag: trigger event syncs
  BASS_MIDI_EVENTS_NORSTATUS = $2000000; // flag: no running status
  BASS_MIDI_EVENTS_CANCEL    = $4000000; // flag: cancel pending events
  BASS_MIDI_EVENTS_TIME      = $8000000; // flag: delta-time info is present
  BASS_MIDI_EVENTS_ABSTIME   = $10000000; // flag: absolute time info is present
  BASS_MIDI_EVENTS_ASYNC     = $20000000; // flag: process asynchronously
  BASS_MIDI_EVENTS_FILTER    = $40000000; // flag: apply filtering
  BASS_MIDI_EVENTS_FLUSH     = $80000000; // flag: flush async events

  // BASS_MIDI_StreamGetChannel special channels
  BASS_MIDI_CHAN_CHORUS      = LongWord(-1);
  BASS_MIDI_CHAN_REVERB      = LongWord(-2);
  BASS_MIDI_CHAN_USERFX      = LongWord(-3);

  // BASS_CHANNELINFO type
  BASS_CTYPE_STREAM_MIDI     = $10d00;

  // Additional attributes
  BASS_ATTRIB_MIDI_PPQN      = $12000;
  BASS_ATTRIB_MIDI_CPU       = $12001;
  BASS_ATTRIB_MIDI_CHANS     = $12002;
  BASS_ATTRIB_MIDI_VOICES    = $12003;
  BASS_ATTRIB_MIDI_VOICES_ACTIVE = $12004;
  BASS_ATTRIB_MIDI_STATE     = $12005;
  BASS_ATTRIB_MIDI_SRC       = $12006;
  BASS_ATTRIB_MIDI_KILL      = $12007;
  BASS_ATTRIB_MIDI_SPEED     = $12008;
  BASS_ATTRIB_MIDI_REVERB    = $12009;
  BASS_ATTRIB_MIDI_VOL       = $1200a;
  BASS_ATTRIB_MIDI_TRACK_VOL = $12100; // + track #

  // Additional tag type
  BASS_TAG_MIDI_TRACK        = $11000; // + track #, track text : array of null-terminated ANSI strings

  // BASS_ChannelGetLength/GetPosition/SetPosition mode
  BASS_POS_MIDI_TICK         = 2; // tick position

  // BASS_MIDI_FontLoadEx flags
  BASS_MIDI_FONTLOAD_NOWAIT  = 1; // don't want for the samples to load
  BASS_MIDI_FONTLOAD_COMPACT = 2; // compact samples
  BASS_MIDI_FONTLOAD_NOLOAD  = 4; // don't load (only compact)
  BASS_MIDI_FONTLOAD_TIME    = 8; // length is in milliseconds
  BASS_MIDI_FONTLOAD_KEEPDEC = 16; // keep decoders

  // BASS_MIDI_FontPack flags
  BASS_MIDI_PACK_NOHEAD      = 1; // don't send a WAV header to the encoder
  BASS_MIDI_PACK_16BIT       = 2; // discard low 8 bits of 24-bit sample data
  BASS_MIDI_PACK_48KHZ       = 4; // set encoding rate to 48000 Hz (else 44100 Hz)

type
  HSOUNDFONT = DWORD;   // soundfont handle

  BASS_MIDI_FONT = record
    font: HSOUNDFONT;   // soundfont
    preset: Integer;    // preset number (-1=all)
    bank: Integer;
  end;
  PBASS_MIDI_FONT = ^BASS_MIDI_FONT;

  BASS_MIDI_FONTEX = record
	font: HSOUNDFONT;   // soundfont
	spreset: Integer;   // source preset number
	sbank: Integer;     // source bank number
	dpreset: Integer;   // destination preset/program number
	dbank: Integer;     // destination bank number
	dbanklsb: Integer;  // destination bank number LSB
  end;
  PBASS_MIDI_FONTEX = ^BASS_MIDI_FONTEX;

  BASS_MIDI_FONTEX2 = record
	font: HSOUNDFONT;   // soundfont
	spreset: Integer;   // source preset number
	sbank: Integer;     // source bank number
	dpreset: Integer;   // destination preset/program number
	dbank: Integer;     // destination bank number
	dbanklsb: Integer;  // destination bank number LSB
	minchan: DWORD;     // minimum channel number
	numchan: DWORD;     // number of channels from minchan
  end;
  PBASS_MIDI_FONTEX2 = ^BASS_MIDI_FONTEX2;

  BASS_MIDI_FONTINFO = record
    name: PAnsiChar;
    copyright: PAnsiChar;
    comment: PAnsiChar;
    presets: DWORD;     // number of presets/instruments
    samsize: DWORD;     // total size (in bytes) of the sample data
    samload: DWORD;     // amount of sample data currently loaded
    samtype: DWORD;     // sample format (CTYPE) if packed
  end;

  BASS_MIDI_MARK = record
    track: DWORD;       // track containing marker
    pos: DWORD;         // marker position
    text: PAnsiChar;    // marker text
  end;
  PBASS_MIDI_MARK = ^BASS_MIDI_MARK;

  BASS_MIDI_EVENT = record
    event: DWORD;       // MIDI_EVENT_xxx
    param: DWORD;
    chan: DWORD;
    tick: DWORD;        // event position (ticks)
    pos: DWORD;         // event position (bytes)
  end;
  PBASS_MIDI_EVENT = ^BASS_MIDI_EVENT;

  BASS_MIDI_DEVICEINFO = record
    name: PAnsiChar;	// description
    id: DWORD;
    flags: DWORD;
  end;

  // callback function types
  MIDIFILTERPROC = function(handle: HSTREAM; track: Integer; event: PBASS_MIDI_EVENT; seeking: BOOL; user: Pointer): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF};
  {
    Event filtering callback function.
    handle : MIDI stream handle
    track  : Track containing the event
    event  : The event
    seeking: TRUE = the event is being processed while seeking, FALSE = it is being played
    user   : The 'user' parameter value given when calling BASS_MIDI_StreamSetFilter
    RETURN : TRUE = process the event, FALSE = drop the event
  }

  MIDIINPROC = procedure(device: DWORD; time: Double; buffer: PByte; length: DWORD; user: Pointer); {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF};
  {
    MIDI input callback function.
    device : MIDI input device
    time   : Timestamp
    buffer : Buffer containing MIDI data
    length : Number of bytes of data
    user   : The 'user' parameter value given when calling BASS_MIDI_InInit
  }

const
{$IFDEF MSWINDOWS}
  bassmididll = 'bassmidi.dll';
{$ENDIF}
{$IFDEF LINUX}
  bassmididll = 'libbassmidi.so';
{$ENDIF}
{$IFDEF ANDROID}
  bassmididll = 'libbassmidi.so';
{$ENDIF}
{$IFDEF MACOS}
  {$IFDEF IOS}
    bassmididll = 'bassmidi.framework/bassmidi';
  {$ELSE}
    bassmididll = 'libbassmidi.dylib';
  {$ENDIF}
{$ENDIF}

function BASS_MIDI_GetVersion: DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;

function BASS_MIDI_StreamCreate(channels,flags,freq:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamCreateFile(mem:BOOL; f:Pointer; offset,length:QWORD; flags,freq:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamCreateURL(url:PChar; offset:DWORD; flags:DWORD; proc:DOWNLOADPROC; user:Pointer; freq:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamCreateFileUser(system,flags:DWORD; var procs:BASS_FILEPROCS; user:Pointer; freq:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamCreateEvents(events:PBASS_MIDI_EVENT; ppqn,flags,freq:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetMark(handle:HSTREAM; type_,index:DWORD; var mark:BASS_MIDI_MARK): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetMarks(handle:HSTREAM; track:Integer; type_:DWORD; marks:PBASS_MIDI_MARK): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamSetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONT; count:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; overload; external bassmididll;
function BASS_MIDI_StreamSetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX; count:DWORD): BOOL; overload;
function BASS_MIDI_StreamGetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONT; count:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; overload; external bassmididll;
function BASS_MIDI_StreamGetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX; count:DWORD): DWORD; overload;
function BASS_MIDI_StreamLoadSamples(handle:HSTREAM): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamEvent(handle:HSTREAM; chan,event,param:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamEvents(handle:HSTREAM; mode:DWORD; events:Pointer; length:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetEvent(handle:HSTREAM; chan,event:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetEvents(handle:HSTREAM; track:Integer; filter:DWORD; events:PBASS_MIDI_EVENT): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetEventsEx(handle:HSTREAM; track:Integer; filter:DWORD; events:PBASS_MIDI_EVENT; start,count:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetPreset(handle:HSTREAM; chan:DWORD; font:PBASS_MIDI_FONT): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamGetChannel(handle:HSTREAM; chan:DWORD): HSTREAM; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_StreamSetFilter(handle:HSTREAM; seeking:BOOL; proc:MIDIFILTERPROC; user:Pointer): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;

function BASS_MIDI_FontInit(fname:PChar; flags:DWORD): HSOUNDFONT; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontInitUser(var procs:BASS_FILEPROCS; user:Pointer; flags:DWORD): HSOUNDFONT; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontFree(handle:HSOUNDFONT): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontGetInfo(handle:HSOUNDFONT; var info:BASS_MIDI_FONTINFO): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontGetPresets(handle:HSOUNDFONT; presets:PLongWord): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontGetPreset(handle:HSOUNDFONT; preset,bank:Integer): PAnsiChar; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontLoad(handle:HSOUNDFONT; preset,bank:Integer): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontLoadEx(handle:HSOUNDFONT; preset,bank:Integer; length,flags:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontUnload(handle:HSOUNDFONT; preset,bank:Integer): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontCompact(handle:HSOUNDFONT): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontPack(handle:HSOUNDFONT; outfile,encoder:PChar; flags:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontUnpack(handle:HSOUNDFONT; outfile:PChar; flags:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontFlags(handle:HSOUNDFONT; flags,mask:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontSetVolume(handle:HSOUNDFONT; volume:Single): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_FontGetVolume(handle:HSOUNDFONT): Single; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;

function BASS_MIDI_ConvertEvents(data:Pointer; length:DWORD; events:PBASS_MIDI_EVENT; count:DWORD; flags:DWORD): DWORD; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;

function BASS_MIDI_InGetDeviceInfo(device:DWORD; var info: BASS_MIDI_DEVICEINFO): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_InInit(device:DWORD; proc:MIDIINPROC; user:Pointer): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_InFree(device:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_InStart(device:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;
function BASS_MIDI_InStop(device:DWORD): BOOL; {$IFDEF MSWINDOWS}stdcall{$ELSE}cdecl{$ENDIF}; external bassmididll;

implementation

function BASS_MIDI_StreamSetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX; count:DWORD): BOOL; overload;
begin
  Result := BASS_MIDI_StreamSetFonts(handle, PBASS_MIDI_FONT(fonts), count or BASS_MIDI_FONT_EX);
end;

function BASS_MIDI_StreamSetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX2; count:DWORD): BOOL; overload;
begin
  Result := BASS_MIDI_StreamSetFonts(handle, PBASS_MIDI_FONT(fonts), count or BASS_MIDI_FONT_EX2);
end;

function BASS_MIDI_StreamGetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX; count:DWORD): DWORD; overload;
begin
  Result := BASS_MIDI_StreamGetFonts(handle, PBASS_MIDI_FONT(fonts), count or BASS_MIDI_FONT_EX);
end;

function BASS_MIDI_StreamGetFonts(handle:HSTREAM; fonts:PBASS_MIDI_FONTEX2; count:DWORD): DWORD; overload;
begin
  Result := BASS_MIDI_StreamGetFonts(handle, PBASS_MIDI_FONT(fonts), count or BASS_MIDI_FONT_EX2);
end;

end.
