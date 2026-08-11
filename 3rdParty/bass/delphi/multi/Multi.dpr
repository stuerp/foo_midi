{
	BASS multiple output example
	Copyright (c) 2001-2008 Un4seen Developments Ltd.

  C++ to Delphi with use API adapted by Evgeny Melnikov
  Required Delphi 7 or above

  
  tested with Delphi7 and Delphi 13
}

program Multi;

{$R 'multi.res' 'multi.rc'}

uses
  Windows,
  Messages,
  CommDlg,
  Bass in '..\Bass.pas';

const
  szFiles = 'streamable files'#0'*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif'#0 +
    'All files'#0'*.*'#0#0;
  szOpen = 'Select a file to play';

var
  win: HWND;
  ofn: OPENFILENAME;
  FileName: array[0..MAX_PATH - 1] of Char;
  outdev: array[0..1] of DWORD = (1, 0);				// output devices
  latency: array[0..1] of DWORD;				// latency of the OutputDevices
  chan: array[0..1] of HSTREAM;			// the streams


//------------------ Auxiliary functions -------------------

function Format(const Format: string; const Args: array of const): string;
var
  I: Integer;
  FormatBuffer: array[0..High(Word)] of Char;
  Arr, Arr1: PDWORD;
  PP: PDWORD;
begin
  Arr := nil;
  if High(Args) >= 0 then
    GetMem(Arr, (High(Args) + 1) * SizeOf(Pointer));
  Arr1 := Arr;
  for I := 0 to High(Args) do
  begin
    PP := @Args[I];
    PP := Pointer(PP^);
    Arr1^ := DWORD(PP);
    inc(Arr1);
  end;
  I := wvsprintf(@FormatBuffer[0], PChar(Format), PChar(Arr));
  SetLength(Result, I);
  Result := FormatBuffer;
  if Arr <> nil then
    FreeMem(Arr);
end;

//---------------------------------------------------------

// display error messages
procedure Error(const es: string);
begin
  MessageBox(win, PChar(Format('%s' + #13#10 + '(error code: %d)', [es, BASS_ErrorGetCode])), 'Error', MB_OK or
    MB_ICONERROR);
end;


//---------------------------------------------------------

// Cloning DSP function
procedure CloneDSP(Handle: HDSP; channel: DWORD; buffer: Pointer; Length: DWORD; user: Pointer); stdcall;
begin
  BASS_StreamPutData(HSTREAM(user), buffer, Length);			// user = clone
end;

//---------------------------------------------------------

procedure InitApp(Wnd: HWND);
var
  info: BASS_INFO;
  di: BASS_DEVICEINFO;
  i: DWORD;
begin
  win := Wnd;
   // get a playable file
  FillChar(ofn, SizeOf(OPENFILENAME), 0);
  FillChar(FileName, SizeOf(FileName), 0);

  ofn.lStructSize := SizeOf(OPENFILENAME);
  ofn.hwndOwner := Wnd;
  ofn.hInstance := hInstance;
  ofn.lpstrTitle := szOpen;
  ofn.lpstrFile := FileName;
  ofn.lpstrFilter := szFiles;
  ofn.nMaxFile := MAX_PATH;
  ofn.Flags := OFN_FILEMUSTEXIST or OFN_HIDEREADONLY or OFN_EXPLORER;
  //
  //add DeviceList to the  Combos
  i := 0;
  while BASS_GetDeviceInfo(i, di) do
  begin
    if di.flags and BASS_DEVICE_ENABLED = BASS_DEVICE_ENABLED then		// enabled, so add it...
    begin
      SendDlgItemMessage(win, 12, CB_ADDSTRING, 0, Integer(PChar(string(di.name))));
      if (i = outdev[0]) then
        SendDlgItemMessage(win, 12, CB_SETCURSEL, i, 0);

      SendDlgItemMessage(win, 13, CB_ADDSTRING, 0, Integer(PChar(string(di.name))));
      if (i = outdev[1]) then
        SendDlgItemMessage(win, 13, CB_SETCURSEL, i, 0);
    end;
    inc(i);
  end;

  // add end
  // initialize output devices
  if not BASS_Init(outdev[0], 44100, BASS_DEVICE_LATENCY, Wnd, nil) then
  begin
    Error('Can''t initialize device 1');
    DestroyWindow(Wnd);
  end;

  BASS_GetInfo(info);
  latency[0] := info.latency;

  if not BASS_Init(outdev[1], 44100, BASS_DEVICE_LATENCY, Wnd, nil) then
  begin
    Error('Can''t initialize device 2');
    DestroyWindow(Wnd);
  end;

  BASS_GetInfo(info);
  latency[1] := info.latency;
end;

//---------------------------------------------------------

function DialogProc(hWnd: hWnd; uMsg: UINT; wParam: wParam; lPARAM: lPARAM): lresult; stdcall;
type
  TBuf = array of Byte;
var
  info: BASS_CHANNELINFO;
  devnx: Integer;
  sel, devn: DWORD;
  c, d: DWORD;
  temp: HSTREAM;
  St: string;
  Buf: TBuf;
  Buf1: array[0..MAX_PATH - 1] of Char;
  Buf2: array[0..MAX_PATH - 1] of Char;
begin
  Result := 0;
  case uMsg of
    WM_INITDIALOG:
      begin
        InitApp(hWnd);
        Result := 1;
        Exit;
      end;
    WM_COMMAND:
      case LOWORD(wParam) of
        10, 11:								// open a file to play on device #1 or device #2
          begin
            devn := LOWORD(wParam) - 10;
            if GetOpenFileName(ofn) then
            begin
              St := FileName;
              BASS_StreamFree(chan[devn]);					// free old stream
              BASS_SetDevice(outdev[devn]);					// set the device to create stream on
              chan[devn] := BASS_StreamCreateFile(0, PChar(St), 0, 0, BASS_SAMPLE_LOOP {$IFDEF UNICODE} or BASS_UNICODE
                {$ENDIF});
              if chan[devn] = 0 then
              begin
                SendDlgItemMessage(hWnd, 10 + devn, WM_SETTEXT, 0, Integer(PChar('click here to open a file...')));
                Error('Can''t play the file');
                Exit;
              end;
              BASS_ChannelPlay(chan[devn], False);				// play new stream
              SendDlgItemMessage(hWnd, 10 + devn, WM_SETTEXT, 0, Integer(PChar(St)));
            end;
          end;
        12, 13: // Device1, Device2
          begin
            if (HIWORD(wParam) = CBN_SELCHANGE) then
            begin  // device selection changed
              sel := SendDlgItemMessage(hWnd, LOWORD(wParam), CB_GETCURSEL, 0, 0); // get the selection
              devn := LOWORD(wParam) - 12;

              if (outdev[devn] = sel) then
              begin
                //exit;
              end;
              if (not BASS_Init(sel, 44100, 0, win, nil)) then
              begin
                Error('Can''''t initialize device');
                SendDlgItemMessage(hWnd, LOWORD(wParam), CB_SETCURSEL, outdev[devn], 0); // Set the selection
              end;
               // initialize new device
              if (chan[devn] > 0) then
                BASS_ChannelSetDevice(chan[devn], sel); // move channel to new device
              BASS_SetDevice(outdev[devn]); // set context to old device
              BASS_Free(); // free it
              outdev[devn] := sel;
            end;
          end;
        15, 16:								// clone on device #1 or device #2
          begin
            devn := LOWORD(wParam) - 15;
            devnx := devn xor 1;
            if not BASS_ChannelGetInfo(chan[devnx], info) then
            begin
              Error('Nothing to clone');
              Exit;
            end;

            BASS_StreamFree(chan[devn]);					// free old stream
            BASS_SetDevice(outdev[devn]);					// set the device to create stream on

            chan[devn] := BASS_StreamCreate(info.freq, info.chans, info.flags, STREAMPROC_PUSH, nil);
            if chan[devn] = 0 then
            begin								// create a "push" stream
              SendDlgItemMessage(hWnd, 10 + devn, WM_SETTEXT, 0, Integer(PChar('click here to open a file...')));
              Error('Can''t create clone');
              Exit;
            end;

            BASS_ChannelLock(chan[devnx], True);				// lock source stream to synchonise buffer contents
            BASS_ChannelSetDSP(chan[devnx], @CloneDSP, Pointer(chan[devn]), 0);	// set DSP to feed data to clone

        // copy buffered data to clone
            d := BASS_ChannelSeconds2Bytes(chan[devn], latency[devn] / 1000); // playback delay
            c := BASS_ChannelGetData(chan[devnx], nil, BASS_DATA_AVAILABLE);
            SetLength(Buf, c);
            c := BASS_ChannelGetData(chan[devnx], Buf, c);
            if c > d then
              BASS_StreamPutData(chan[devn], Pointer(DWORD(Buf) + d), c - d);

            BASS_ChannelLock(chan[devnx], False);				// unlock source stream
            BASS_ChannelPlay(chan[devn], False);				// play clone
            SendDlgItemMessage(hWnd, 10 + devn, WM_SETTEXT, 0, Integer(PChar('clone')));
          end;
        30:								// swap channel devices
          begin
        // swap handles
            temp := chan[0];
            chan[0] := chan[1];
            chan[1] := temp;

        // swap text
            SendDlgItemMessage(hWnd, 10, WM_GETTEXT, SizeOf(Buf1), Integer(@Buf1[0]));
            SendDlgItemMessage(hWnd, 11, WM_GETTEXT, SizeOf(Buf2), Integer(@Buf2[0]));
            SendDlgItemMessage(hWnd, 10, WM_SETTEXT, 0, Integer(@Buf2[0]));
            SendDlgItemMessage(hWnd, 11, WM_SETTEXT, 0, Integer(@Buf1[0]));

        // update the channel devices
            BASS_ChannelSetDevice(chan[0], outdev[0]);
            BASS_ChannelSetDevice(chan[1], outdev[1]);
          end;
        IDCANCEL:
          DestroyWindow(hWnd);
      end;
    WM_CLOSE:
      DestroyWindow(hWnd);
    WM_DESTROY:
      begin
      // release both devices
        BASS_SetDevice(outdev[0]);
        BASS_Free;
        BASS_SetDevice(outdev[1]);
        BASS_Free;
      end;
  end;
end;

//---------------------------------------------------------



begin
  // check the correct BASS was loaded
  if HIWORD(BASS_GetVersion) <> BASSVERSION then
  begin
    MessageBox(0, 'An incorrect version of BASS.DLL was loaded', 'Error', MB_OK or MB_ICONERROR);
    Halt(BASS_ERROR_VERSION);
  end;


  // display the window
  DialogBox(hInstance, MakeIntResource(1000), 0, @DialogProc);
end.

