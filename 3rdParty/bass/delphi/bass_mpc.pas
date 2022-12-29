Unit BASS_MPC;

interface

uses windows, bass;

const
  // Additional tags available from BASS_StreamGetTags
  BASS_TAG_APE		= 6;	// APE tags
  // BASS_CHANNELINFO type
  BASS_CTYPE_STREAM_MPC        = $10a00;


const
  bassmpcdll = 'bass_mpc.dll';


function BASS_MPC_StreamCreateFile(mem: BOOL; f: Pointer; offset, length: QWORD; flags: DWORD): HSTREAM; stdcall; external bassmpcdll;
function BASS_MPC_StreamCreateURL(URL:PAnsiChar; offset:DWORD; flags:DWORD; proc:DOWNLOADPROC; user:Pointer): HSTREAM; stdcall; external bassmpcdll;
function BASS_MPC_StreamCreateFileUser(system, flags: DWORD; var procs: BASS_FILEPROCS; user: Pointer): HSTREAM; stdcall; external bassmpcdll;

implementation

end.