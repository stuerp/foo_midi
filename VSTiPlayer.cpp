#include "VSTiPlayer.h"

#include <shared.h>

// #define LOG_EXCHANGE

VSTiPlayer::VSTiPlayer()
{
	hProcess = NULL;
	hThread = NULL;
	hChildStd_IN_Rd = NULL;
	hChildStd_IN_Wr = NULL;
	hChildStd_OUT_Rd = NULL;
	hChildStd_OUT_Wr = NULL;
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
	delete [] sVendor;
	delete [] sProduct;
}

bool VSTiPlayer::LoadVST(const char * path)
{
	sPlugin = path;

	return process_create();
}

bool VSTiPlayer::process_create()
{
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if ( !CreatePipe( &hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0 ) ||
		!SetHandleInformation( hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) ||
		!CreatePipe( &hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0 ) ||
		!SetHandleInformation( hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0 ) )
	{
		process_terminate();
		return false;
	}

	pfc::string8 szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	szCmdLine.truncate( szCmdLine.scan_filename() );
	szCmdLine += "vsthost.exe";
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

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo = {0};

	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.hStdError = hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = hChildStd_OUT_Wr;
	siStartInfo.hStdInput = hChildStd_IN_Rd;
	//siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

	if ( !CreateProcess( NULL, (LPTSTR)(LPCTSTR) pfc::stringcvt::string_os_from_utf8( szCmdLine ), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo ) )
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

	SetPriorityClass( hProcess, GetPriorityClass( GetCurrentProcess() ) );
	SetThreadPriority( hThread, GetThreadPriority( GetCurrentThread() ) );

	t_uint32 code = process_read_code();

	if ( code != 0 )
	{
		process_terminate();
		return false;
	}

	t_uint32 vendor_string_length = process_read_code();
	t_uint32 product_string_length = process_read_code();
	uVendorVersion = process_read_code();
	uUniqueId = process_read_code();
	uNumOutputs = process_read_code();

	delete [] sVendor;
	delete [] sProduct;

	sVendor = new char[ vendor_string_length + 1 ];
	sProduct = new char[ product_string_length + 1 ];

	process_read_bytes( sVendor, vendor_string_length );
	process_read_bytes( sProduct, product_string_length );

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
	if ( hChildStd_IN_Rd ) CloseHandle( hChildStd_IN_Rd );
	if ( hChildStd_IN_Wr ) CloseHandle( hChildStd_IN_Wr );
	if ( hChildStd_OUT_Rd ) CloseHandle( hChildStd_OUT_Rd );
	if ( hChildStd_OUT_Wr ) CloseHandle( hChildStd_OUT_Wr );
	hProcess = NULL;
	hThread = NULL;
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

void VSTiPlayer::process_read_bytes( void * in, t_uint32 size )
{
	if ( process_running() && size )
	{
		DWORD dwRead;
		if ( !ReadFile( hChildStd_OUT_Rd, in, size, &dwRead, NULL ) || dwRead < size )
		{
			memset( in, 0xFF, size );
#ifdef LOG_EXCHANGE
			TCHAR logfile[MAX_PATH];
			_stprintf_s( logfile, _T("C:\\temp\\log2\\bytes_%08u.err"), exchange_count++ );
			FILE * f = _tfopen( logfile, _T("wb") );
			_ftprintf( f, _T("Wanted %u bytes, got %u"), size, dwRead );
			fclose( f );
#endif
		}
		else
		{
#ifdef LOG_EXCHANGE
			TCHAR logfile[MAX_PATH];
			_stprintf_s( logfile, _T("C:\\temp\\log2\\bytes_%08u.in"), exchange_count++ );
			FILE * f = _tfopen( logfile, _T("wb") );
			fwrite( in, 1, size, f );
			fclose( f );
#endif
		}
	}
	else memset( in, 0xFF, size );
}

t_uint32 VSTiPlayer::process_read_code()
{
	t_uint32 code;
	process_read_bytes( &code, sizeof(code) );
	return code;
}

void VSTiPlayer::process_write_bytes( const void * out, t_uint32 size )
{
	if ( process_running() )
	{
		DWORD dwWritten;
		WriteFile( hChildStd_IN_Wr, out, size, &dwWritten, NULL );
#ifdef LOG_EXCHANGE
		TCHAR logfile[MAX_PATH];
		_stprintf_s( logfile, _T("C:\\temp\\log2\\bytes_%08u.out"), exchange_count++ );
		FILE * f = _tfopen( logfile, _T("wb") );
		fwrite( out, 1, size, f );
		fclose( f );
#endif
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

bool VSTiPlayer::Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, bool clean_flag)
{
	assert(!mStream.get_count());

	midi_file.serialize_as_stream( subsong, mStream, mSysexMap, clean_flag );

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
		t_size size;
		mSysexMap.get_entry( n, data, size );
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
