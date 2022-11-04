unit Unit1;
{
 *  BASS Recording example for Delphi  by Chris Trösken
 This Demo should be compatible from Delphi 6 - Delphi (11, Alexandria)
 The Demo where Based/translated from the C Demo from Un4seen.com.

}
interface

uses
  Windows, Messages, SysUtils, Classes, Graphics,
  Controls, Forms, Dialogs, Bass, StdCtrls, UITypes, ComCtrls, ExtCtrls, MMSystem;

type
  WAVHDR = packed record
    riff: array [0 .. 3] of AnsiChar;
    len: DWord;
    cWavFmt: array [0 .. 7] of AnsiChar;
    dwHdrLen: DWord;
    wFormat: Word;
    wNumChannels: Word;
    dwSampleRate: DWord;
    dwBytesPerSec: DWord;
    wBlockAlign: Word;
    wBitsPerSample: Word;
    cData: array [0 .. 3] of AnsiChar;
    dwDataLen: DWord;
  end;

  TForm1 = class(TForm)
    cb1: TComboBox;
    cb2: TComboBox;
    Label1: TLabel;
    TrackBar1: TTrackBar;
    Label2: TLabel;
    bPlay: TButton;
    bRecord: TButton;
    bSave: TButton;
    lPos: TLabel;
    Bevel1: TBevel;
    Timer1: TTimer;
    SaveDialog: TSaveDialog;
    procedure FormCreate(Sender: TObject);
    procedure cb1Change(Sender: TObject);
    procedure cb2Change(Sender: TObject);
    procedure TrackBar1Change(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure bSaveClick(Sender: TObject);
    procedure bRecordClick(Sender: TObject);
    procedure bPlayClick(Sender: TObject);
  private
    { Private-Deklarationen }

    function InitDevice(Device: integer): boolean;
    procedure UpdateInputInfo;
    procedure StopRecording;
    procedure StartRecording;
  public
    { Public-Deklarationen }
  end;

var
  Form1: TForm1;
  win: HWND = 0;
  WaveStream: TMemoryStream ;
  rchan: HRecord = 0;
  Chan: HStream = 0;
  WaveHdr: WAVHDR; // WAV header

implementation

{$R *.dfm}

(* This is called while recording audio *)
function RecordingCallback(Handle: HRecord; buffer: Pointer; length: DWord; user: Pointer): boolean; stdcall;
begin
  // Copy new buffer contents to the memory buffer
  WaveStream.Write(buffer^, length);
  {
    in the C Example
    if ((reclen mod BUFSTEP) + length >= BUFSTEP) ....
		recbuf = realloc(recbuf, ((reclen + length) / BUFSTEP + 1) * BUFSTEP);
    this is in Delphi not Needed TMemorystream will (if needed ) increase the size of the memory buffer self
  }
  // Allow recording to continue
  Result := True;
end;

procedure Error(msg: PChar; ShowAllways: boolean = false);
var
  s: Widestring;
begin
  if ShowAllways or (BASS_ErrorGetCode <> 0) then // for testing if needed
  begin
    s := msg + #13#10 + #13#10 + '(Error Code: ' + inttostr(BASS_ErrorGetCode) + ')';
    MessageBox(win, PWideChar(s), 'Error', MB_ICONEXCLAMATION);
  end;
end;

procedure TForm1.UpdateInputInfo;
var
  it: DWord;
  level: single;
  info: BASS_DEVICEINFO;
begin
  it := BASS_RecordGetInput(cb2.ItemIndex, level);
  TrackBar1.Position := Round(level * 100); // set the level slider
  Label2.Caption := IntToStr(TrackBar1.Position);

  case (it and BASS_INPUT_TYPE_MASK) of
    BASS_INPUT_TYPE_DIGITAL:
      Label1.Caption := 'digital';
    BASS_INPUT_TYPE_LINE:
      Label1.Caption := 'line-in';
    BASS_INPUT_TYPE_MIC:
      Label1.Caption := 'microphone';
    BASS_INPUT_TYPE_SYNTH:
      Label1.Caption := 'midi synth';
    BASS_INPUT_TYPE_CD:
      Label1.Caption := 'analog cd';
    BASS_INPUT_TYPE_PHONE:
      Label1.Caption := 'telephone';
    BASS_INPUT_TYPE_SPEAKER:
      Label1.Caption := 'pc speaker';
    BASS_INPUT_TYPE_WAVE:
      Label1.Caption := 'wave/pcm';
    BASS_INPUT_TYPE_AUX:
      Label1.Caption := 'aux';
    BASS_INPUT_TYPE_ANALOG:
      Label1.Caption := 'analog';
  else
    Label1.Caption := 'undefined';
  end;
  BASS_RecordGetDeviceInfo(BASS_RecordGetDevice(), info);
  if (info.flags and BASS_DEVICE_LOOPBACK) > 0 then
    Label1.Caption := 'loopback';
end;

function TForm1.InitDevice(Device: integer): boolean;
var
  i: integer;
  dName: PAnsiChar;
  level: single;
begin
  Result := True;
  BASS_RecordFree(); // free current device (and recording channel) if there is one
  if not(BASS_RecordInit(Device)) then // initalize new device
  begin
    Error('Can''''t initialize recording device');
    Result := false;
  end;
  i := 0;
  cb2.Clear;
  // first set it as default device
  dName := BASS_RecordGetInputName(i);
  while dName <> nil do
  begin
    cb2.Items.Add(Widestring(dName));
    // is this one currently "on"?
    if (BASS_RecordGetInput(i, level) and BASS_INPUT_OFF) = 0 then
    begin
      cb2.ItemIndex := i;
      TrackBar1.Position := Round(level / 100); // Get the Current Level of the Device
    end;
    Inc(i);
    dName := BASS_RecordGetInputName(i);
  end;
  // enable the selected input
  cb2.OnChange(self);

  UpdateInputInfo; // update info
end;

procedure TForm1.Timer1Timer(Sender: TObject);
begin
  if (rchan > 0) then
  begin
    if (not rchan = 1) then
    begin
      if (BASS_ChannelIsActive(rchan) = 0) then
      begin
        StopRecording();
        Error('The recording stopped');
       // exit;
      end;
    end;
    lPos.Caption := IntToStr(WaveStream.Size);
  end
  else if (Chan > 0) then
  begin
    if (BASS_ChannelIsActive(Chan)) > 0 then // playing
      lPos.Caption := IntToStr(BASS_ChannelGetPosition(Chan, BASS_POS_BYTE)) + ' / ' +
        IntToStr(BASS_ChannelGetLength(Chan, BASS_POS_BYTE))
    else
      lPos.Caption := IntToStr(BASS_ChannelGetLength(Chan, BASS_POS_BYTE));
  end;
end;

procedure TForm1.TrackBar1Change(Sender: TObject);
begin
  if not BASS_RecordSetInput(cb1.ItemIndex, BASS_INPUT_ON, (TrackBar1.Position / 100)) then
    BASS_RecordSetInput(-1, 0, TrackBar1.Position / 100);
  Label2.Caption := IntToStr(TrackBar1.Position);
end;

procedure TForm1.bPlayClick(Sender: TObject);
begin
if not BASS_ChannelPlay(chan, false) then
 Error('Error can"t play the Stream');
end;

procedure TForm1.bRecordClick(Sender: TObject);
begin
if BASS_ChannelIsActive(rchan) <> 0 then
    StopRecording
  else
    StartRecording;
end;

procedure TForm1.bSaveClick(Sender: TObject);
begin
 if SaveDialog.Execute then
    WaveStream.SaveToFile(SaveDialog.FileName);
end;

procedure TForm1.cb1Change(Sender: TObject);
var
  Newrchan: HRecord;
begin
  if (rchan > 0) then
    rchan := 1;
  // special handle (real handles always have highest bit set) to prevent timer ending the recording
  if InitDevice(cb1.ItemIndex) then
  begin
    if (rchan > 0) then
    begin // continue recording on the new device...
      Newrchan := BASS_RecordStart(44100, 2, 0, @RecordingCallback, nil);
      if (Newrchan = 0) then
        Error(' Couldn"t start recording')
      else
        rchan := Newrchan;
    end;
  end;
end;

procedure TForm1.cb2Change(Sender: TObject);
var
  r: boolean;
  i: integer;
begin
  r := True;
  i := 0;
  // first disable all inputs, then...
  while r do
  begin
    r := BASS_RecordSetInput(i, BASS_INPUT_OFF, -1);
    Inc(i);
  end;
  BASS_RecordSetInput(cb1.ItemIndex, BASS_INPUT_ON, -1);

  // TrackBar1.Enabled := cb2.ItemIndex > -1;
  UpdateInputInfo();
end;

procedure TForm1.FormCreate(Sender: TObject);
var
  C: DWord;
  di: BASS_DEVICEINFO;
  Def: integer;
begin
  win := Handle;
  Def := -1;
  // check the correct BASS was loaded
  if (HIWORD(BASS_GetVersion) <> BASSVERSION) then
  begin
    MessageBox(0, 'An incorrect version of BASS.DLL was loaded', nil, MB_ICONERROR);
    Halt;
  end;

  InitDevice(cb1.ItemIndex);

  if not BASS_Init(-1, 44100, 0, Handle, nil) then
  begin
    BASS_Free();
    MessageDlg('Cannot start default device!', mtError, [mbOk], 0);
    Halt;
  end;
  // catch possible Recording Devices
  C := 0;
  while (BASS_RecordGetDeviceInfo(C, di)) do
  begin
    cb1.Items.Add(Widestring(di.name));
    if (di.flags and BASS_DEVICE_DEFAULT) > 0 then // got the default device
      Def := C;
    Inc(C);
  end;

  cb1.ItemIndex := Def;
  WaveStream := TMemoryStream.Create;
  UpdateInputInfo();

end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
  BASS_RecordFree();
  BASS_Free();
  WaveStream.Free;
end;

procedure TForm1.StopRecording;
var
  i: integer;
begin
  BASS_ChannelStop(rchan);
  rchan := 0;
  bRecord.Caption := 'Record';
  // complete the WAV header
  WaveStream.Position := 4;
  i := WaveStream.Size - 8;
  WaveStream.Write(i, 4);
  i := i - $24;
  WaveStream.Position := 40;
  WaveStream.Write(i, 4);
  WaveStream.Position := 0;
  // create a stream from the recorded data
  chan := BASS_StreamCreateFile(True, WaveStream.Memory, 0, WaveStream.Size, 0);
  if chan <> 0 then
  begin
    // enable "Play" & "Save" buttons
    bPlay.Enabled := True;
    bSave.Enabled := True;
  end
  else
    MessageDlg('Error creating stream from recorded data!', mtError, [mbOk], 0);
end;

procedure TForm1.StartRecording;
begin
  if cb1.ItemIndex < 0 then
    exit;
  if WaveStream.Size > 0 then
  begin
    // free old recording
    BASS_StreamFree(rchan);
    WaveStream.Clear;
  end;
  // generate header for WAV file
  with WaveHdr do
  begin
    riff := 'RIFF';
    len := 36;
    cWavFmt := 'WAVEfmt ';
    dwHdrLen := 16;
    wFormat := 1;
    wNumChannels := 2;
    dwSampleRate := 44100;
    wBlockAlign := 4;
    dwBytesPerSec := 176400;
    wBitsPerSample := 16;
    cData := 'data';
    dwDataLen := 0;
  end;
  WaveStream.Write(WaveHdr, SizeOf(WAVHDR));
  // start recording @ 44100hz 16-bit stereo
  rchan := BASS_RecordStart(44100, 2, 0, @RecordingCallback, nil);

  if rchan = 0 then
  begin
    MessageDlg('Couldn''t start recording!' + 'ErrorCode ' + inttostr(BASS_ErrorGetCode()), mtError, [mbOk], 0);
    WaveStream.Clear;
  end
  else
  begin
    bRecord.Caption := 'Stop';
    bPlay.Enabled := false;
    bSave.Enabled := false;
  end;
end;

end.
