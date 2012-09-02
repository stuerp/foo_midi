#include "VSTiPlayer.h"

#include <shared.h>

// #define LOG_EXCHANGE

VSTiPlayer::VSTiPlayer()
{
	bInitialized = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
	hChildStd = NULL;
	sName = NULL;
	sVendor = NULL;
	sProduct = NULL;
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uNumOutputs = 0;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
}

VSTiPlayer::~VSTiPlayer()
{
	process_terminate();
	delete [] sName;
	delete [] sVendor;
	delete [] sProduct;
}

static WORD getwordle(BYTE *pData)
{
	return (WORD)(pData[0] | (((WORD)pData[1]) << 8));
}

static DWORD getdwordle(BYTE *pData)
{
	return pData[0] | (((DWORD)pData[1]) << 8) | (((DWORD)pData[2]) << 16) | (((DWORD)pData[3]) << 24);
}

unsigned VSTiPlayer::test_plugin_platform() {
#define iMZHeaderSize (0x40)
#define iPEHeaderSize (4 + 20 + 224)

	BYTE peheader[iPEHeaderSize];
	DWORD dwOffsetPE;

	pfc::string8 plugin = "file://";
	plugin += sPlugin;

	file::ptr f;
	abort_callback_dummy m_abort;

	try
	{
		filesystem::g_open( f, plugin, filesystem::open_mode_read, m_abort );

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
	sPlugin = path;

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

bool VSTiPlayer::connect_pipe( HANDLE hPipe )
{
	OVERLAPPED ol = {};
	ol.hEvent = hReadEvent;
	ResetEvent( hReadEvent );
	if ( !ConnectNamedPipe( hPipe, &ol ) )
	{
		DWORD error = GetLastError();
		if ( error == ERROR_PIPE_CONNECTED ) return true;
		if ( error != ERROR_IO_PENDING ) return false;

		if ( WaitForSingleObject( hReadEvent, 10000 ) == WAIT_TIMEOUT ) return false;
	}
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

	pfc::string8 pipe_name;
	if ( !create_pipe_name( pipe_name ) )
	{
		process_terminate();
		return false;
	}

	pfc::stringcvt::string_os_from_utf8 pipe_name_os( pipe_name );

	HANDLE hPipe = CreateNamedPipe( pipe_name_os, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr );
	DuplicateHandle( GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd, 0, FALSE, DUPLICATE_SAME_ACCESS );

	pfc::string8 szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	szCmdLine.truncate( szCmdLine.scan_filename() );
	szCmdLine += (uPluginPlatform == 64) ? "vsthost64.exe" : "vsthost32.exe";
	szCmdLine += "\" \"";
	szCmdLine += sPlugin;
	szCmdLine += "\" ";

	unsigned sum = 0;

	pfc::stringcvt::string_os_from_utf8 plugin_os( sPlugin );
	const TCHAR * ch = plugin_os.get_ptr();
	while ( *ch )
	{
		sum += (TCHAR)( *ch++ * 820109 );
	}

	szCmdLine += pfc::format_int( sum, 0, 16 );

	szCmdLine += " ";
	szCmdLine += pipe_name.get_ptr() + 9;

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo = {0};

	siStartInfo.cb = sizeof(siStartInfo);

	if ( !CreateProcess( NULL, (LPTSTR)(LPCTSTR) pfc::stringcvt::string_os_from_utf8( szCmdLine ), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo ) )
	{
		process_terminate();
		return false;
	}

	hProcess = piProcInfo.hProcess;
	hThread = piProcInfo.hThread;

#ifdef NDEBUG
	SetPriorityClass( hProcess, GetPriorityClass( GetCurrentProcess() ) );
	SetThreadPriority( hThread, GetThreadPriority( GetCurrentThread() ) );
#endif

	if ( !connect_pipe( hChildStd ) )
	{
		process_terminate();
		return false;
	}

	t_uint32 code = process_read_code();

	if ( code != 0 )
	{
		process_terminate();
		return false;
	}

	t_uint32 name_string_length = process_read_code();
	t_uint32 vendor_string_length = process_read_code();
	t_uint32 product_string_length = process_read_code();
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
	if ( hProcess )
	{
		process_write_code( 0 );
		WaitForSingleObject( hProcess, 5000 );
		TerminateProcess( hProcess, 0 );
		CloseHandle( hThread );
		CloseHandle( hProcess );
	}
	if ( hChildStd ) CloseHandle( hChildStd );
	if ( hReadEvent ) CloseHandle( hReadEvent );
	if ( bInitialized ) CoUninitialize();
	bInitialized = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
	hChildStd = NULL;
}

bool VSTiPlayer::process_running()
{
	if ( hProcess && WaitForSingleObject( hProcess, 0 ) == WAIT_TIMEOUT ) return true;
	return false;
}

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

static void ProcessPendingMessages()
{
	MSG msg = {};
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) DispatchMessage( &msg );
}

t_uint32 VSTiPlayer::process_read_bytes_pass( void * out, t_uint32 size )
{
	OVERLAPPED ol = {};
	ol.hEvent = hReadEvent;
	ResetEvent( hReadEvent );
	DWORD bytesDone;
	SetLastError( NO_ERROR );
	if ( ReadFile( hChildStd, out, size, &bytesDone, &ol ) ) return bytesDone;
	if ( GetLastError() != ERROR_IO_PENDING ) return 0;

	const HANDLE handles[1] = {hReadEvent};
	SetLastError( NO_ERROR );
	DWORD state;
	for (;;)
	{
		state = MsgWaitForMultipleObjects( _countof( handles ), handles, FALSE, INFINITE, QS_ALLEVENTS );
		if ( state == WAIT_OBJECT_0 + _countof( handles ) ) ProcessPendingMessages();
		else break;
	}

	if ( state == WAIT_OBJECT_0 && GetOverlappedResult( hChildStd, &ol, &bytesDone, TRUE ) ) return bytesDone;

#if _WIN32_WINNT >= 0x600
	CancelIoEx( hChildStd, &ol );
#else
	CancelIo( hChildStd );
#endif

	return 0;
}

void VSTiPlayer::process_read_bytes( void * out, t_uint32 size )
{
	if ( process_running() && size )
	{
		t_uint8 * ptr = (t_uint8 *) out;
		t_uint32 done = 0;
		while ( done < size )
		{
			t_uint32 delta = process_read_bytes_pass( ptr + done, size - done );
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

t_uint32 VSTiPlayer::process_read_code()
{
	t_uint32 code;
	process_read_bytes( &code, sizeof(code) );
	return code;
}

void VSTiPlayer::process_write_bytes( const void * in, t_uint32 size )
{
	if ( process_running() )
	{
		if ( size == 0 ) return;
		DWORD bytesWritten;
		if ( !WriteFile( hChildStd, in, size, &bytesWritten, NULL ) || bytesWritten < size ) process_terminate();
	}
}

void VSTiPlayer::process_write_code( t_uint32 code )
{
	process_write_bytes( &code, sizeof(code) );
}

void VSTiPlayer::getVendorString(pfc::string_base & out)
{
	out = sVendor;
}

void VSTiPlayer::getProductString(pfc::string_base & out)
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

void VSTiPlayer::getChunk( pfc::array_t<t_uint8> & out )
{
	process_write_code( 1 );

	t_uint32 code = process_read_code();

	if ( code == 0 )
	{
		t_uint32 size = process_read_code();

		out.set_count( size );

		process_read_bytes( out.get_ptr(), size );
	}
	else process_terminate();
}

void VSTiPlayer::setChunk( const void * in, unsigned size )
{
	process_write_code( 2 );
	process_write_code( size );
	process_write_bytes( in, size );
	t_uint32 code = process_read_code();
	if ( code != 0 ) process_terminate();
}

bool VSTiPlayer::hasEditor()
{
	process_write_code( 3 );
	t_uint32 code = process_read_code();
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
	t_uint32 code = process_read_code();
	if ( code != 0 ) process_terminate();
}

void VSTiPlayer::setSampleRate(unsigned rate)
{
	if (mStream.get_count())
	{
		for (UINT i = 0; i < mStream.get_count(); i++)
		{
			mStream[ i ].m_timestamp = MulDiv( mStream[ i ].m_timestamp, rate, uSampleRate );
		}
	}

	if (uTimeCurrent)
	{
		uTimeCurrent = MulDiv(uTimeCurrent, rate, uSampleRate);
	}

	if (uTimeEnd)
	{
		uTimeEnd = MulDiv(uTimeEnd, rate, uSampleRate);
	}

	if (uTimeLoopStart)
	{
		uTimeLoopStart = MulDiv(uTimeLoopStart, rate, uSampleRate);
	}

	uSampleRate = rate;

	process_write_code( 5 );
	process_write_code( sizeof(t_uint32) );
	process_write_code( rate );
	
	t_uint32 code = process_read_code();
	if ( code != 0 ) process_terminate();
}

unsigned VSTiPlayer::getChannelCount()
{
	return uNumOutputs;
}

bool VSTiPlayer::Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags)
{
	assert(!mStream.get_count());

	midi_file.serialize_as_stream( subsong, mStream, mSysexMap, clean_flags );

	if (mStream.get_count())
	{
		uStreamPosition = 0;
		uTimeCurrent = 0;

		uLoopMode = loop_mode;

		if (uLoopMode & loop_mode_enable)
		{
			uTimeLoopStart = midi_file.get_timestamp_loop_start( subsong, true );
			unsigned uTimeLoopEnd = midi_file.get_timestamp_loop_end( subsong, true );
			uTimeEnd = midi_file.get_timestamp_end( subsong, true );

			if ( uTimeLoopStart != ~0 || uTimeLoopEnd != ~0 )
			{
				uLoopMode |= loop_mode_force;
			}

			if ( uTimeLoopStart != ~0 )
			{
				for ( unsigned i = 0; i < mStream.get_count(); ++i )
				{
					if ( mStream[ i ].m_timestamp >= uTimeLoopStart )
					{
						uStreamLoopStart = i;
						break;
					}
				}
			}
			else uStreamLoopStart = ~0;

			if ( uTimeLoopEnd != ~0 )
			{
				uTimeEnd = uTimeLoopEnd;
			}

			if (!(uLoopMode & loop_mode_force)) uTimeEnd += 1000;
			else
			{
				UINT i;
				unsigned char note_on[128 * 16];
				memset( note_on, 0, sizeof( note_on ) );
				for (i = 0; i < mStream.get_count(); i++)
				{
					if (mStream[ i ].m_timestamp > uTimeEnd) break;
					UINT ev = mStream[ i ].m_event & 0x800000F0;
					if ( ev == 0x90 || ev == 0x80 )
					{
						UINT port = ( mStream[ i ].m_event & 0x7F000000 ) >> 24;
						UINT ch = mStream[ i ].m_event & 0x0F;
						UINT note = ( mStream[ i ].m_event >> 8 ) & 0x7F;
						UINT on = ( ev == 0x90 ) && ( mStream[ i ].m_event & 0xFF0000 );
						UINT bit = 1 << port;
						note_on [ ch * 128 + note ] = ( note_on [ ch * 128 + note ] & ~bit ) | ( bit * on );
					}
				}
				mStream.set_count( i );
				for ( UINT j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] )
					{
						for ( UINT k = 0; k < 8; k++ )
						{
							if ( note_on[ j ] & ( 1 << k ) )
							{
								mStream.append_single( midi_stream_event( uTimeEnd, ( k << 24 ) + ( j >> 7 ) + ( j & 0x7F ) * 0x100 + 0x90 ) );
							}
						}
					}
				}
			}
		}
		else uTimeEnd = midi_file.get_timestamp_end( subsong, true ) + 1000;

		if (uSampleRate != 1000)
		{
			unsigned rate = uSampleRate;
			uSampleRate = 1000;
			setSampleRate(rate);
		}

		return true;
	}

	return false;
}

unsigned VSTiPlayer::Play(audio_sample * out, unsigned count)
{
	assert(mStream.get_count());

	if ( !process_running() ) return 0;

	DWORD done = 0;

	if ( uSamplesRemaining )
	{
		DWORD todo = uSamplesRemaining;
		if (todo > count) todo = count;
		render( out, todo );
		uSamplesRemaining -= todo;
		done += todo;
		uTimeCurrent += todo;
	}

	while (done < count)
	{
		DWORD todo = uTimeEnd - uTimeCurrent;
		if (todo > count - done) todo = count - done;

		DWORD time_target = todo + uTimeCurrent;
		UINT stream_end = uStreamPosition;

		while (stream_end < mStream.get_count() && mStream[stream_end].m_timestamp < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			for (; uStreamPosition < stream_end; uStreamPosition++)
			{
				midi_stream_event * me = mStream.get_ptr() + uStreamPosition;
				
				DWORD samples_todo = me->m_timestamp - uTimeCurrent;
				if ( samples_todo )
				{
					if ( samples_todo > count - done )
					{
						uSamplesRemaining = samples_todo - ( count - done );
						samples_todo = count - done;
					}
					render( out + done * 2, samples_todo );
					done += samples_todo;

					if ( uSamplesRemaining )
					{
						uTimeCurrent = me->m_timestamp;
						return done;
					}
				}

				send_event( me->m_event );

				uTimeCurrent = me->m_timestamp;
			}
		}

		if ( done < count )
		{
			DWORD samples_todo;
			if ( uStreamPosition < mStream.get_count() ) samples_todo = mStream[uStreamPosition].m_timestamp;
			else samples_todo = uTimeEnd;
			samples_todo -= uTimeCurrent;
			if ( samples_todo > count - done ) samples_todo = count - done;
			render( out + done * 2, samples_todo );
			done += samples_todo;
		}

		uTimeCurrent = time_target;

		if (uTimeCurrent >= uTimeEnd)
		{
			if ( uStreamPosition < mStream.get_count() )
			{
				for (; uStreamPosition < mStream.get_count(); uStreamPosition++)
				{
					send_event( mStream[ uStreamPosition ].m_event );
				}
			}

			if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
			{
				if (uStreamLoopStart == ~0)
				{
					uStreamPosition = 0;
					uTimeCurrent = 0;
				}
				else
				{
					uStreamPosition = uStreamLoopStart;
					uTimeCurrent = uTimeLoopStart;
				}
			}
			else break;
		}
	}

	return done;
}

void VSTiPlayer::Seek(unsigned sample)
{
	if (sample >= uTimeEnd)
	{
		if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
		{
			while (sample >= uTimeEnd) sample -= uTimeEnd - uTimeLoopStart;
		}
		else
		{
			sample = uTimeEnd;
		}
	}

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		process_write_code( 6 );
		
		t_uint32 code = process_read_code();
		if ( code != 0 )
		{
			process_terminate();
			return;
		}
	}

	uTimeCurrent = sample;

	pfc::array_t<midi_stream_event> filler;

	UINT stream_start = uStreamPosition;

	for (; uStreamPosition < mStream.get_count() && mStream[uStreamPosition].m_timestamp < uTimeCurrent; uStreamPosition++);

	uSamplesRemaining = mStream[uStreamPosition].m_timestamp - uTimeCurrent;

	if (uStreamPosition > stream_start)
	{
		filler.set_size( uStreamPosition - stream_start );
		memcpy(filler.get_ptr(), &mStream[stream_start], sizeof(midi_stream_event) * (uStreamPosition - stream_start));

		UINT i, j;
		midi_stream_event * me = filler.get_ptr();

		for (i = 0, stream_start = uStreamPosition - stream_start; i < stream_start; i++)
		{
			midi_stream_event & e = me[i];
			if ((e.m_event & 0x800000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
			{
				if ((e.m_event & 0x0F) == 9) // hax
				{
					e.m_event = 0;
					continue;
				}
				DWORD m = (e.m_event & 0x7F00FF0F) | 0x80; // note off
				DWORD m2 = e.m_event & 0x7F00FFFF; // also note off
				for (j = i + 1; j < stream_start; j++)
				{
					midi_stream_event & e2 = me[j];
					if ((e2.m_event & 0xFF00FFFF) == m || e2.m_event == m2)
					{
						// kill 'em
						e.m_event = 0;
						e2.m_event = 0;
						break;
					}
				}
			}
		}

		for (i = 0, j = 0; i < stream_start; i++)
		{
			if (me[i].m_event)
			{
				if (i != j) me[j] = me[i];
				j++;
			}
		}

		if (!j) return;

		for ( i = 0; i < j; i++ )
		{
			send_event( me[i].m_event );
		}
	}
}

void VSTiPlayer::send_event( DWORD b )
{
	if (!(b & 0x80000000))
	{
		process_write_code( 7 );
		process_write_code( b );
	}
	else
	{
		UINT n = b & 0xffffff;
		const t_uint8 * data;
		t_size size, port;
		mSysexMap.get_entry( n, data, size, port );
		process_write_code( 8 );
		process_write_code( size );
		process_write_bytes( data, size );
	}
	t_uint32 code = process_read_code();
	if ( code != 0 ) process_terminate();
}

void VSTiPlayer::render( audio_sample * out, unsigned count )
{
	process_write_code( 9 );
	process_write_code( count );
	t_uint32 code = process_read_code();
	if ( code != 0 )
	{
		process_terminate();
		memset( out, 0, sizeof(audio_sample) * count * uNumOutputs );
		return;
	}
	assert(sizeof(audio_sample) == sizeof(float));

	while ( count )
	{
		unsigned count_to_do = 4096 * uNumOutputs;
		if ( count_to_do > count ) count_to_do = count;
		process_read_bytes( out, sizeof(audio_sample) * count_to_do * uNumOutputs );
		out += count_to_do * uNumOutputs;
		count -= count_to_do;
	}
}
