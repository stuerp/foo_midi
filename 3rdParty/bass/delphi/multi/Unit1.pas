unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Bass;

type
  TForm1 = class(TForm)
    GroupBox1: TGroupBox;
    GroupBox2: TGroupBox;
    Button1: TButton;
    Button2: TButton;
    OpenDialog1: TOpenDialog;
    btnSwap: TButton;
    Button3: TButton;
    Button4: TButton;
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure Button1Click(Sender: TObject);
    procedure btnSwapClick(Sender: TObject);
    procedure Button3Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    outdev: array[0..1] of DWORD;
    chan: array[0..1] of HSTREAM;
  end;

var
  Form1: TForm1;

implementation

uses Unit2;

{$R *.dfm}

procedure CloneDSP(handle: HDSP; channel: DWORD; buffer: Pointer; length: DWORD; user: Pointer); stdcall;
begin
	BASS_StreamPutData(HSTREAM(User), Buffer, Length); // user = clone
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion) <> BASSVERSION) then
	begin
		MessageBox(0,'An incorrect version of BASS.DLL was loaded',nil,MB_ICONERROR);
		Halt;
	end;

  Application.CreateForm(TForm2, Form2);
  Form2.Caption := 'select output device #1';
  outdev[0] := Form2.ShowModal;
  Form2.Free;
  Application.CreateForm(TForm2, Form2);
  Form2.Caption := 'select output device #2';
  outdev[1] := Form2.ShowModal;
  Form2.Free;

  if (not BASS_Init(outdev[0], 44100, 0, Handle, nil)) then
  begin
    MessageBox(0, PChar('Cant''t initialize device 1' + #13#10 + 'Error #' + IntToStr(BASS_ErrorGetCode)), nil, MB_ICONERROR);
    Halt;
  end;
  if (not BASS_Init(outdev[1], 44100, 0, Handle, nil)) then
  begin
    MessageBox(0, PChar('Cant''t initialize device 2' + #13#10 + 'Error #' + IntToStr(BASS_ErrorGetCode)), nil, MB_ICONERROR);
    Halt;
  end; 
end;

procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  BASS_SetDevice(outdev[0]);
  BASS_Free;
  BASS_SetDevice(outdev[1]);
  BASS_Free;

  Action := caFree;
end;

procedure TForm1.Button1Click(Sender: TObject);
var
  curdev: Integer;
begin
  if OpenDialog1.Execute then
  begin
    Curdev := TButton(Sender).Tag;
    BASS_StreamFree(chan[curdev]);
    BASS_SetDevice(outdev[curdev]);
    Chan[curdev] := BASS_StreamCreateFile(False, PChar(OpenDialog1.FileName), 0, 0, BASS_SAMPLE_LOOP {$IFDEF UNICODE} or BASS_UNICODE {$ENDIF});
    if (chan[curdev] = 0) then
    begin
      TButton(Sender).Caption := 'click here to open a file...';
      MessageBox(0, 'Can''t play the file', nil, MB_ICONERROR);
      Exit;
    end;
    TButton(Sender).Caption := OpenDialog1.FileName;
    BASS_ChannelPlay(chan[curdev], False);
  end;  
end;

procedure TForm1.btnSwapClick(Sender: TObject);
var
 tmp : HStream;
 tmpText : string;
begin
  tmp:=chan[0];
  chan[0]:=chan[1];
  chan[1]:=tmp;
  // swap Text
  tmpText := Button1.Caption;
  Button1.Caption := Button2.Caption;
  Button2.Caption := tmpText; 
  BASS_ChannelSetDevice(chan[0],outdev[0]);
  BASS_ChannelSetDevice(chan[1],outdev[1]);
end;

procedure TForm1.Button3Click(Sender: TObject);
var
  Buf: Pointer;
  C: DWORD;
  DevN: Integer;
  Info: BASS_CHANNELINFO;

  procedure SetButtonCaption(const Caption: string);
  begin
    if DevN = 0 then
      Button1.Caption := 'Clone'
    else
      Button2.Caption := 'Clone';
  end;    

begin
  DevN := TButton(Sender).Tag;
  if not BASS_ChannelGetInfo(Chan[DevN xor 1], Info) then
  begin
    MessageBox(0, 'Nothing to clone', nil, MB_ICONERROR);
    Exit;
  end;
  BASS_StreamFree(chan[devn]); // free old stream
  BASS_SetDevice(outdev[devn]); // set the device to create stream on
  chan[devn] := BASS_StreamCreate(Info.freq, Info.Chans, Info.flags, STREAMPROC_PUSH, nil); // create a "push" stream;
  if chan[devn] = 0 then
  begin
    SetButtonCaption('click here to open a file...');
    MessageBox(0, 'Can''t create clone', nil, MB_ICONERROR);
    Exit;
  end;
  BASS_ChannelLock(chan[DevN xor 1], TRUE); // lock source stream to synchonise buffer contents
  BASS_ChannelSetDSP(chan[DevN xor 1], @CloneDSP, Pointer(chan[devn]), 0); // set DSP to feed data to clone
  // copy buffered data to clone
  C := BASS_ChannelGetData(Chan[DevN xor 1], nil, BASS_DATA_AVAILABLE);
  Buf := AllocMem(C);
  try
    c := BASS_ChannelGetData(chan[DevN xor 1], buf, c);
    BASS_StreamPutData(chan[DevN], buf, c);
  finally
    FreeMem(buf);
  end;
  BASS_ChannelLock(chan[devn xor 1], FALSE); // unlock source stream
  BASS_ChannelPlay(chan[DevN], FALSE); // play clone
  SetButtonCaption('Clone');
end;

end.
