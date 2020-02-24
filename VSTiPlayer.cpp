#include "VSTiPlayer.h"

#include <foobar2000.h>

// #define LOG_EXCHANGE

VSTiPlayer::VSTiPlayer() : MIDIPlayer()
{
	bInitialized = false;
	bTerminating = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
 	hChildStd_IN_Rd = NULL;
 	hChildStd_IN_Wr = NULL;
 	hChildStd_OUT_Rd = NULL;
 	hChildStd_OUT_Wr = NULL;
	sName = NULL;
	sVendor = NULL;
	sProduct = NULL;
	uNumOutputs = 0;
}

VSTiPlayer::~VSTiPlayer()
{
	process_terminate();
	delete [] sName;
	delete [] sVendor;
	delete [] sProduct;
}

static uint16_t getwordle(uint8_t *pData)
{
	return (uint16_t)(pData[0] | (((uint16_t)pData[1]) << 8));
}

static uint32_t getdwordle(uint8_t *pData)
{
	return pData[0] | (((uint32_t)pData[1]) << 8) | (((uint32_t)pData[2]) << 16) | (((uint32_t)pData[3]) << 24);
}

unsigned VSTiPlayer::test_plugin_platform() {
#define iMZHeaderSize (0x40)
#define iPEHeaderSize (4 + 20 + 224)

	uint8_t peheader[iPEHeaderSize];
	uint32_t dwOffsetPE;

	std::string plugin = "file://";
	plugin += sPlugin;

	file::ptr f;
	abort_callback_dummy m_abort;

	try
	{
		filesystem::g_open( f, plugin.c_str(), filesystem::open_mode_read, m_abort );

		f->read_object( peheader, iMZHeaderSize, m_abort );
		if ( getwordle(peheader) != 0x5A4D ) return 0;
		dwOffsetPE = getdwordle( peheader + 0x3c );
		f->seek( dwOffsetPE, m_abort );
		f->read_object( peheader, iPEHeaderSize, m_abort );
		if ( getdwordle( peheader ) != 0x00004550 ) return 0;
		switch ( getwordle( peheader + 4 ) ) {
		case 0x014C: return 32;
		case 0x8664: return 64;
		}
	} catch (...) { }

	return 0;
}

bool VSTiPlayer::LoadVST(const char * path)
{
	if (!path || !path[0]) return false;

	if ( path != sPlugin ) sPlugin = path;

	uPluginPlatform = test_plugin_platform();

	if ( !uPluginPlatform ) return false;

	return process_create();
}

static bool create_pipe_name( pfc::string_base & out )
{
	GUID guid;
	if ( FAILED( CoCreateGuid( &guid ) ) ) return false;

	out = "\\\\.\\pipe\\";
	out += pfc::print_guid( guid );

	return true;
}

bool VSTiPlayer::process_create()
{
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if ( !bInitialized )
	{
		if ( FAILED( CoInitialize( NULL ) ) ) return false;
		bInitialized = true;
	}

	hReadEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	pfc::string8 pipe_name_in, pipe_name_out;
 	if ( !create_pipe_name( pipe_name_in ) || !create_pipe_name( pipe_name_out ) )
 	{
 		process_terminate();
 		return false;
 	}
 
 	pfc::stringcvt::string_os_from_utf8 pipe_name_in_os( pipe_name_in ), pipe_name_out_os( pipe_name_out );
 
 	HANDLE hPipe = CreateNamedPipe( pipe_name_in_os, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr );
 	if ( hPipe == INVALID_HANDLE_VALUE )
 	{
 		process_terminate();
 		return false;
 	}
 	hChildStd_IN_Rd = CreateFile( pipe_name_in_os, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL );
 	DuplicateHandle( GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_IN_Wr, 0, FALSE, DUPLICATE_SAME_ACCESS );
 	CloseHandle( hPipe );
 
 	hPipe = CreateNamedPipe( pipe_name_out_os, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr );
 	if ( hPipe == INVALID_HANDLE_VALUE )
 	{
 		process_terminate();
 		return false;
 	}
 	hChildStd_OUT_Wr = CreateFile( pipe_name_out_os, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL );
 	DuplicateHandle( GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_OUT_Rd, 0, FALSE, DUPLICATE_SAME_ACCESS );
 	CloseHandle( hPipe );
 
 	std::string szCmdLine = "\"";
 	szCmdLine += core_api::get_my_full_path();
	size_t slash = szCmdLine.find_last_of( '\\' );
	if ( slash != std::string::npos )
		szCmdLine.erase( szCmdLine.begin() + slash + 1, szCmdLine.end() );
 	szCmdLine += (uPluginPlatform == 64) ? "64\\vsthost64.exe" : "32\\vsthost32.exe";
 	szCmdLine += "\" \"";
 	szCmdLine += sPlugin;
 	szCmdLine += "\" ";
 
 	unsigned sum = 0;
 
 	pfc::stringcvt::string_os_from_utf8 plugin_os( sPlugin.c_str() );
 	const TCHAR * ch = plugin_os.get_ptr();
 	while ( *ch )
 	{
 		sum += (TCHAR)( *ch++ * 820109 );
 	}
 
 	szCmdLine += pfc::format_int( sum, 0, 16 );
 
 	PROCESS_INFORMATION piProcInfo;
 	STARTUPINFO siStartInfo = {0};
 
 	siStartInfo.cb = sizeof(siStartInfo);
 	siStartInfo.hStdInput = hChildStd_IN_Rd;
 	siStartInfo.hStdOutput = hChildStd_OUT_Wr;
 	siStartInfo.hStdError = GetStdHandle( STD_ERROR_HANDLE );
 	//siStartInfo.wShowWindow = SW_HIDE;
 	siStartInfo.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;
 
 	if ( !CreateProcess( NULL, (LPTSTR)(LPCTSTR) pfc::stringcvt::string_os_from_utf8( szCmdLine.c_str() ), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo ) )
 	{
 		process_terminate();
 		return false;
 	}
 
 	// Close remote handles so pipes will break when process terminates
 	CloseHandle( hChildStd_OUT_Wr );
 	CloseHandle( hChildStd_IN_Rd );
 	hChildStd_OUT_Wr = NULL;
 	hChildStd_IN_Rd = NULL;

	hProcess = piProcInfo.hProcess;
	hThread = piProcInfo.hThread;

#ifdef NDEBUG
	SetPriorityClass( hProcess, GetPriorityClass( GetCurrentProcess() ) );
	SetThreadPriority( hThread, GetThreadPriority( GetCurrentThread() ) );
#endif

	uint32_t code = process_read_code();

	if ( code != 0 )
	{
		process_terminate();
		return false;
	}

	uint32_t name_string_length = process_read_code();
	uint32_t vendor_string_length = process_read_code();
	uint32_t product_string_length = process_read_code();
	uVendorVersion = process_read_code();
	uUniqueId = process_read_code();
	uNumOutputs = process_read_code();

	delete [] sName;
	delete [] sVendor;
	delete [] sProduct;

	sName = new char[ name_string_length + 1 ];
	sVendor = new char[ vendor_string_length + 1 ];
	sProduct = new char[ product_string_length + 1 ];

	process_read_bytes( sName, name_string_length );
	process_read_bytes( sVendor, vendor_string_length );
	process_read_bytes( sProduct, product_string_length );

	sName[ name_string_length ] = 0;
	sVendor[ vendor_string_length ] = 0;
	sProduct[ product_string_length ] = 0;

	return true;
}

void VSTiPlayer::process_terminate()
{
	if (bTerminating) return;

	bTerminating = true;

	if ( hProcess )
	{
		process_write_code( 0 );
		WaitForSingleObject( hProcess, 5000 );
		TerminateProcess( hProcess, 0 );
		CloseHandle( hThread );
		CloseHandle( hProcess );
	}
 	if ( hChildStd_IN_Rd ) CloseHandle( hChildStd_IN_Rd );
 	if ( hChildStd_IN_Wr ) CloseHandle( hChildStd_IN_Wr );
 	if ( hChildStd_OUT_Rd ) CloseHandle( hChildStd_OUT_Rd );
 	if ( hChildStd_OUT_Wr ) CloseHandle( hChildStd_OUT_Wr );
	if ( hReadEvent ) CloseHandle( hReadEvent );
	if ( bInitialized ) CoUninitialize();
	bInitialized = false;
	bTerminating = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
 	hChildStd_IN_Rd = NULL;
 	hChildStd_IN_Wr = NULL;
 	hChildStd_OUT_Rd = NULL;
 	hChildStd_OUT_Wr = NULL;
}

bool VSTiPlayer::process_running()
{
	if ( hProcess && WaitForSingleObject( hProcess, 0 ) == WAIT_TIMEOUT ) return true;
	return false;
}

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

#ifdef MESSAGE_PUMP
static void ProcessPendingMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
#endif

uint32_t VSTiPlayer::process_read_bytes_pass( void * out, uint32_t size )
{
	OVERLAPPED ol = {};
	ol.hEvent = hReadEvent;
	ResetEvent( hReadEvent );
	DWORD bytesDone;
	SetLastError( NO_ERROR );
	if ( ReadFile( hChildStd_OUT_Rd, out, size, &bytesDone, &ol ) ) return bytesDone;
	if ( ::GetLastError() != ERROR_IO_PENDING ) return 0;

	const HANDLE handles[1] = {hReadEvent};
	SetLastError( NO_ERROR );
	DWORD state;
#ifdef MESSAGE_PUMP
	for (;;)
	{
		state = MsgWaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE, QS_ALLEVENTS);
		if (state == WAIT_OBJECT_0 + _countof(handles)) ProcessPendingMessages();
		else break;
	}
#else
	state = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
#endif

	if ( state == WAIT_OBJECT_0 && GetOverlappedResult( hChildStd_OUT_Rd, &ol, &bytesDone, TRUE ) ) return bytesDone;

#if _WIN32_WINNT >= 0x600
	CancelIoEx( hChildStd_OUT_Rd, &ol );
#else
	CancelIo( hChildStd_OUT_Rd );
#endif

	return 0;
}

void VSTiPlayer::process_read_bytes( void * out, uint32_t size )
{
	if ( process_running() && size )
	{
		uint8_t * ptr = (uint8_t *) out;
		uint32_t done = 0;
		while ( done < size )
		{
			uint32_t delta = process_read_bytes_pass( ptr + done, size - done );
			if ( delta == 0 )
			{
				memset( out, 0xFF, size );
				break;
			}
			done += delta;
		}
	}
	else memset( out, 0xFF, size );
}

uint32_t VSTiPlayer::process_read_code()
{
	uint32_t code;
	process_read_bytes( &code, sizeof(code) );
	return code;
}

void VSTiPlayer::process_write_bytes( const void * in, uint32_t size )
{
	if ( process_running() )
	{
		if ( size == 0 ) return;
		DWORD bytesWritten;
		if ( !WriteFile( hChildStd_IN_Wr, in, size, &bytesWritten, NULL ) || bytesWritten < size ) process_terminate();
	}
}

void VSTiPlayer::process_write_code( uint32_t code )
{
	process_write_bytes( &code, sizeof(code) );
}

void VSTiPlayer::getVendorString(std::string & out)
{
	out = sVendor;
}

void VSTiPlayer::getProductString(std::string & out)
{
	out = sProduct;
}

long VSTiPlayer::getVendorVersion()
{
	return uVendorVersion;
}

long VSTiPlayer::getUniqueID()
{
	return uUniqueId;
}

void VSTiPlayer::getChunk( std::vector<uint8_t> & out )
{
	process_write_code( 1 );

	t_uint32 code = process_read_code();

	if ( code == 0 )
	{
		uint32_t size = process_read_code();

		out.resize( size );

		if ( size )
			process_read_bytes( &out[0], size );
	}
	else process_terminate();
}

void VSTiPlayer::setChunk( const void * in, unsigned long size )
{
	if ( blChunk.size() == 0 || ( blChunk.size() == size && size != 0 && in != (const void*)&blChunk[0] ) )
	{
		blChunk.resize( size );
		if ( size )
			memcpy( &blChunk[0], in, size );
	}

	process_write_code( 2 );
	process_write_code( size );
	process_write_bytes( in, size );
	uint32_t code = process_read_code();
	if ( code != 0 ) process_terminate();
}

bool VSTiPlayer::hasEditor()
{
	process_write_code( 3 );
	uint32_t code = process_read_code();
	if ( code != 0 )
	{
		process_terminate();
		return false;
	}
	code = process_read_code();
	return code != 0;
}

void VSTiPlayer::displayEditorModal()
{
	process_write_code( 4 );
	uint32_t code = process_read_code();
	if ( code != 0 ) process_terminate();
}

void VSTiPlayer::shutdown()
{
	process_terminate();
}

bool VSTiPlayer::startup()
{
	if ( process_running() ) return true;

	if ( !LoadVST( sPlugin.c_str() ) ) return false;

	if ( blChunk.size() )
		setChunk( &blChunk[0], blChunk.size() );

	process_write_code( 5 );
	process_write_code( sizeof(uint32_t) );
	process_write_code( uSampleRate );
	
	uint32_t code = process_read_code();
	if ( code != 0 ) process_terminate();

	return true;
}

unsigned VSTiPlayer::getChannelCount()
{
	return uNumOutputs;
}

void VSTiPlayer::send_event( uint32_t b )
{
	if (!(b & 0x80000000))
	{
		process_write_code( 7 );
		process_write_code( b );
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		process_write_code( 8 );
		process_write_code( size );
		process_write_bytes( data, size );
	}
	t_uint32 code = process_read_code();
	if ( code != 0 ) process_terminate();
}

void VSTiPlayer::render( float * out, unsigned long count )
{
	process_write_code( 9 );
	process_write_code( count );
	uint32_t code = process_read_code();
	if ( code != 0 )
	{
		process_terminate();
		memset( out, 0, sizeof(float) * count * uNumOutputs );
		return;
	}

	while ( count )
	{
		unsigned count_to_do = 4096 * uNumOutputs;
		if ( count_to_do > count ) count_to_do = count;
		process_read_bytes( out, sizeof(float) * count_to_do * uNumOutputs );
		out += count_to_do * uNumOutputs;
		count -= count_to_do;
	}
}
