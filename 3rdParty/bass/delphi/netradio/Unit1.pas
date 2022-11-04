unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Bass, ExtCtrls, mmsystem,math;

const
  WM_INFO_UPDATE = WM_USER + 101;
  // HLS definitions (copied from BASSHLS.pas)
  BASS_SYNC_HLS_SEGMENT = $10300;
  BASS_TAG_HLS_EXTINF = $14000;

type

  TForm1 = class(TForm)
    Panel1: TPanel;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    Label2: TLabel;
    Button1: TButton;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Button5: TButton;
    Button6: TButton;
    Button7: TButton;
    Button8: TButton;
    Button9: TButton;
    Button10: TButton;
    GroupBox3: TGroupBox;
    Label6: TLabel;
    ed_ProxyServer: TEdit;
    cbDirectConnection: TCheckBox;
    Label7: TLabel;
    Edit1: TEdit;
    Button11: TButton;
    GroupBox2: TGroupBox;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure Button1Click(Sender: TObject);
  private
    { Private declarations }

  public
    { Public declarations }
    procedure WndProc(var Msg: TMessage); override;

  end;

var
  Form1: TForm1;

  cthread: DWORD = 0;
  chan: HSTREAM = 0;
  win: hwnd;
  req: DWORD = 0; // request number/counter
  FTimerId: DWORD = 0;
  icy: PAnsiChar = nil;
  progress: integer = 0;

implementation

const
  urls: array [0 .. 9] of String = ( // preset stream URLs
    'http://stream-dc1.radioparadise.com/rp_192m.ogg', 'http://www.radioparadise.com/m3u/mp3-32.m3u',
    'http://network.absoluteradio.co.uk/core/audio/mp3/live.pls?service=a8bb',
    'http://network.absoluteradio.co.uk/core/audio/aacplus/live.pls?service=a8', 'http://somafm.com/secretagent.pls',
    'http://somafm.com/secretagent32.pls', 'http://somafm.com/suburbsofgoa.pls', 'http://somafm.com/suburbsofgoa32.pls',
    'http://ai-radio.org/256.ogg', 'http://ai-radio.org/48.aacp');

{$R *.dfm}
  { display error messages }

procedure Error(es: string);
begin
  MessageBox(win, PChar(es + #13#10 + '(error code: ' + IntToStr(BASS_ErrorGetCode) + ')'), nil, 0);
end;

{ update stream title from metadata }

procedure DoMeta();
var
  meta: PAnsiChar;
  Artist, Title: string;
  p, p1, p2: integer;
begin
  meta := BASS_ChannelGetTags(chan, BASS_TAG_META);
  if (meta <> nil) then
  begin
    // got Shoutcast metadata
    p := Pos('StreamTitle=', String(AnsiString(meta)));
    if (p = 0) then
      Exit;
    p := p + 13;
    SendMessage(win, WM_INFO_UPDATE, 7, DWORD(PAnsiChar(AnsiString(Copy(meta, p, Pos(';', String(meta)) - p - 1)))));
    meta := nil;
  end
  else
    meta := BASS_ChannelGetTags(chan, BASS_TAG_OGG);
  if meta <> nil then
  begin
    // got Icecast/OGG tags
    p1 := Pos('artist=', string(meta));
    p2 := Pos('title=', string(meta));
    if p1 > 0 then
      Artist := Copy(string(meta), p1 + 7, Length(string(meta)));
    if p2 > 0 then
      Title := Copy(string(meta), p2 + 6, Length(string(meta)));
    if p1 > 0 then
      SendMessage(win, WM_INFO_UPDATE, 7, DWORD(PAnsiChar(AnsiString(Format('%s - %s', [Artist, Title])))))
    else
      SendMessage(win, WM_INFO_UPDATE, 7, DWORD(PAnsiChar(AnsiString(Format('%s', [Title])))));
  end
  else
  begin
    meta := BASS_ChannelGetTags(chan, BASS_TAG_HLS_EXTINF);
    if meta <> '' then
    begin
      // got HLS segment info
      SendMessage(win, WM_INFO_UPDATE, 7,
        DWORD(PAnsiChar((Copy(meta, Pos(',', string(meta)) + 1, Length(string(meta)))))));
    end;
  end;
end;

procedure MetaSync(handle: HSYNC; channel, data: DWORD; user: Pointer); stdcall;
begin
  DoMeta();
end;

procedure StatusProc(buffer: Pointer; len: DWORD; user: Pointer); stdcall;
begin
  if (buffer <> nil) and (len = 0) and (DWORD(user) = req) then
    SendMessage(win, WM_INFO_UPDATE, 8, DWORD(PAnsiChar(buffer)));
end;

procedure StallSync(handle: HSYNC; channel, data: DWORD; user: Pointer); stdcall;
begin
  if (data = 0) then // stalled
    FTimerId := SetTimer(win, 0, 50, nil); // start buffer monitoring
end;

procedure EndSync(handle: HSYNC; channel, data: DWORD; user: Pointer); stdcall;
begin
  KillTimer(win, FTimerId); // stop buffer monitoring
  SendMessage(win, WM_INFO_UPDATE, 1, 0); // reset Labels
end;

function OpenURL(url: PWideChar): integer;
var
  R: DWORD;
  C: HSTREAM;
  FLock: TRtlCriticalSection;
begin
  Result := 0;
  InitializeCriticalSection(FLock);
  EnterCriticalSection(FLock); // make sure only 1 thread at a time can do the following
  try
    inc(req);
    R := req;
  finally
    LeaveCriticalSection(FLock);
  end;

  KillTimer(win, FTimerId);
  BASS_StreamFree(chan); // close old stream

  SendMessage(win, WM_INFO_UPDATE, 0, 0); // reset the Labels and trying connecting

  //
  C := BASS_StreamCreateURL(url, 0, BASS_STREAM_BLOCK or BASS_STREAM_STATUS or BASS_STREAM_AUTOFREE or BASS_UNICODE,
    @StatusProc, Pointer(R));

  EnterCriticalSection(FLock);
  try
    if (R <> req) then
    begin // there is a newer request, discard this stream
      LeaveCriticalSection(FLock);
      DeleteCriticalSection(FLock);
      if C <> 0 then
        BASS_StreamFree(C);
      Exit;
    end;
    chan := C; // this is now the current stream
  finally
    LeaveCriticalSection(FLock);
    DeleteCriticalSection(FLock);
  end;
  if (chan = 0) then
  begin
    // lets catch the error here inside the Thread and send it to the WndProc
    SendMessage(win, WM_INFO_UPDATE, 1, BASS_ErrorGetCode()); // Oops Error
  end
  else
  begin
    FTimerId := SetTimer(win, 0, 25, nil);
  end;
  BASS_ChannelSetSync(chan, BASS_SYNC_META, 0, MetaSync, nil); // Shoutcast
  BASS_ChannelSetSync(chan, BASS_SYNC_OGG_CHANGE, 0, MetaSync, nil); // Vorbis/OGG
  BASS_ChannelSetSync(chan, BASS_SYNC_HLS_SEGMENT, 0, MetaSync, nil); // HLS
  // set sync for stalling/buffering
  BASS_ChannelSetSync(chan, BASS_SYNC_STALL, 0, StallSync, nil);
  // set sync for end of stream
  BASS_ChannelSetSync(chan, BASS_SYNC_END, 0, EndSync, nil);
  // play it!
  BASS_ChannelPlay(chan, FALSE);

  cthread := 0;
end;

procedure TForm1.WndProc(var Msg: TMessage);
// to be threadsave we are passing all Canvas Stuff(e.g. Labels) to this messages
begin
  inherited;
  case Msg.Msg of
    WM_TIMER:
      begin
        // Display Tags
        if (BASS_ChannelIsActive(chan) = BASS_ACTIVE_PLAYING) then
        begin
          KillTimer(win, FTimerId); // finished buffering, stop monitoring
          // get the broadcast name and bitrate
          icy := BASS_ChannelGetTags(chan, BASS_TAG_ICY);
          if (icy = nil) then
            icy := BASS_ChannelGetTags(chan, BASS_TAG_HTTP); // no ICY tags, try HTTP
          if (icy <> nil) then
            while (icy^ <> #0) do
            begin
              if (Copy(icy, 1, 9) = 'icy-name:') then
                SendMessage(win, WM_INFO_UPDATE, 3, DWORD(PAnsiChar(Copy(icy, 10, MaxInt))))
              else if (Copy(icy, 1, 7) = 'icy-br:') then
                SendMessage(win, WM_INFO_UPDATE, 4, DWORD(PAnsiChar('bitrate: ' + Copy(icy, 8, MaxInt))));
              icy := icy + Length(icy) + 1;
            end;
          // get the stream title and set sync for subsequent titles
          DoMeta();
        end
        else
        begin
          // monitor buffering progress
          if BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFERING) > 0 then
          // this check will prevent that the Buffer display do not start with 100
             SendMessage(win, WM_INFO_UPDATE, 2,(100 - (integer(BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFERING)))));
          // show the Progess value in the label
        end;
      end;
    WM_INFO_UPDATE:
      begin
        case Msg.WParam of
          0:
            begin
              Label4.Caption := 'connecting...';
              Label3.Caption := '';
              Label5.Caption := '';
            end;
          1:
            begin
              Label4.Caption := 'not playing';
              Error('Can''t play the stream');
            end;
          2:
            Label4.Caption := Format('buffering... %d%%', [Msg.LParam]);
          3:
            Label4.Caption := String(PAnsiChar(Msg.LParam));
          4:
            Label5.Caption := String(PAnsiChar(Msg.LParam));
          5:
            Label5.Caption := String(PAnsiChar(Msg.LParam));
          6:
            Label3.Caption := String(PAnsiChar(Msg.LParam));
          7:
            Label3.Caption := String(PAnsiChar(Msg.LParam));
          8:
            Label5.Caption := String(PAnsiChar(Msg.LParam));
        end;
      end;
  end;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  // check the correct BASS was loaded
  win := handle;
  if (HIWORD(BASS_GetVersion) <> BASSVERSION) then
  begin
    MessageBox(0, 'An incorrect version of BASS.DLL was loaded', nil, MB_ICONERROR);
    Halt;
  end;
  if (not BASS_Init(-1, 44100, 0, handle, nil)) then
  begin
    Error('Can''t initialize device');
    Halt;
  end;
  BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing

  BASS_PluginLoad('bass_aac.dll', BASS_Unicode); // load BASS_AAC (if present) for AAC support on older Windows
  BASS_PluginLoad('bassflac.dll', BASS_Unicode); // load BASSFLAC (if present) for FLAC support
  BASS_PluginLoad('basshls.dll', BASS_Unicode); // load BASSHLS (if present) for HLS support

end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
  BASS_Free;
end;

procedure TForm1.Button1Click(Sender: TObject);
var
  ThreadId: Cardinal;
begin
  if (cthread <> 0) then
    MessageBeep(0)
  else
  begin
    if cbDirectConnection.Checked then
      BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, nil) // disable proxy
    else
      BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY or BASS_UNICODE, PWideChar(ed_ProxyServer.Text)); // set proxy server
    // open URL in a new thread (so that main thread is free)
    if (Sender as TButton).Tag in [0 .. 9] then
      cthread := BeginThread(nil, 0, @OpenURL, PWideChar(urls[TButton(Sender).Tag]), 0, ThreadId)
    else
      cthread := BeginThread(nil, 0, @OpenURL, PWideChar(Edit1.Text), 0, ThreadId); // custum url
  end;
end;

end.
