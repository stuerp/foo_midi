// vsthost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

enum
{
	BUFFER_SIZE = 4096,
};

// #define LOG_EXCHANGE

bool need_idle = false;

struct myVstEvent
{
	struct myVstEvent * next;
	unsigned port;
	union
	{
		VstMidiEvent midiEvent;
		VstMidiSysexEvent sysexEvent;
	} ev;
} * evChain = NULL, * evTail = NULL;

void freeChain()
{
	myVstEvent * ev = evChain;
	while ( ev )
	{
		myVstEvent * next = ev->next;
		if ( ev->ev.sysexEvent.type == kVstSysExType ) free( ev->ev.sysexEvent.sysexDump );
		free( ev );
		ev = next;
	}
	evChain = NULL;
	evTail = NULL;
}

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

void put_bytes( const void * out, uint32_t size )
{
	DWORD dwWritten;
	WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), out, size, &dwWritten, NULL );
#ifdef LOG_EXCHANGE
	TCHAR logfile[MAX_PATH];
	_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.out"), exchange_count++ );
	FILE * f = _tfopen( logfile, _T("wb") );
	fwrite( out, 1, size, f );
	fclose( f );
#endif
}

void put_code( uint32_t code )
{
	put_bytes( &code, sizeof(code) );
}

void get_bytes( void * in, uint32_t size )
{
	DWORD dwRead;
	if ( !ReadFile( GetStdHandle( STD_INPUT_HANDLE ), in, size, &dwRead, NULL ) || dwRead < size )
	{
		memset( in, 0, size );
#ifdef LOG_EXCHANGE
		TCHAR logfile[MAX_PATH];
		_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.err"), exchange_count++ );
		FILE * f = _tfopen( logfile, _T("wb") );
		_ftprintf( f, _T("Wanted %u bytes, got %u"), size, dwRead );
		fclose( f );
#endif
	}
	else
	{
#ifdef LOG_EXCHANGE
		TCHAR logfile[MAX_PATH];
		_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.in"), exchange_count++ );
		FILE * f = _tfopen( logfile, _T("wb") );
		fwrite( in, 1, size, f );
		fclose( f );
#endif
	}
}

uint32_t get_code()
{
	uint32_t code;
	get_bytes( &code, sizeof(code) );
	return code;
}

void getChunk( AEffect * pEffect, std::vector<uint8_t> & out )
{
	out.resize( 0 );
	uint32_t unique_id = pEffect->uniqueID;
	append_be( out, unique_id );
	bool type_chunked = !!( pEffect->flags & effFlagsProgramChunks );
	append_be( out, type_chunked );
	if ( !type_chunked )
	{
		uint32_t num_params = pEffect->numParams;
		append_be( out, num_params );
		for ( unsigned i = 0; i < num_params; ++i ) 
		{
			float parameter = pEffect->getParameter( pEffect, i );
			append_be( out, parameter );
		}
	}
	else
	{
		void * chunk;
		uint32_t size = pEffect->dispatcher( pEffect, effGetChunk, 0, 0, &chunk, 0 );
		append_be( out, size );
		size_t chunk_size = out.size();
		out.resize( chunk_size + size );
		memcpy( &out[ chunk_size ], chunk, size );
	}
}

void setChunk( AEffect * pEffect, std::vector<uint8_t> const& in )
{
	unsigned size = in.size();
	if ( pEffect && size )
	{
		const uint8_t * inc = in.data();
		uint32_t effect_id;
		retrieve_be( effect_id, inc, size );
		if ( effect_id != pEffect->uniqueID ) return;
		bool type_chunked;
		retrieve_be( type_chunked, inc, size );
		if ( type_chunked != !!( pEffect->flags & effFlagsProgramChunks ) ) return;
		if ( !type_chunked )
		{
			uint32_t num_params;
			retrieve_be( num_params, inc, size );
			if ( num_params != pEffect->numParams ) return;
			for ( unsigned i = 0; i < num_params; ++i )
			{
				float parameter;
				retrieve_be( parameter, inc, size );
				pEffect->setParameter( pEffect, i, parameter );
			}
		}
		else
		{
			uint32_t chunk_size;
			retrieve_be( chunk_size, inc, size );
			if ( chunk_size > size ) return;
			pEffect->dispatcher( pEffect, effSetChunk, 0, chunk_size, ( void * ) inc, 0 );
		}
	}
}

struct ERect
{
	short top;
	short left;
	short bottom;
	short right;
};

struct MyDLGTEMPLATE: DLGTEMPLATE
{
	WORD ext[3];
	MyDLGTEMPLATE ()
	{ memset (this, 0, sizeof(*this)); };
};

INT_PTR CALLBACK EditorProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	AEffect* effect;
	switch(msg)
	{
	case WM_INITDIALOG :
		{
			SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
			effect = (AEffect*)lParam;
			SetWindowText (hwnd, L"VST Editor");
			SetTimer (hwnd, 1, 20, 0);
			if(effect)
			{
				effect->dispatcher (effect, effEditOpen, 0, 0, hwnd, 0);
				ERect* eRect = 0;
				effect->dispatcher (effect, effEditGetRect, 0, 0, &eRect, 0);
				if(eRect)
				{
					int width = eRect->right - eRect->left;
					int height = eRect->bottom - eRect->top;
					if(width < 50)
						width = 50;
					if(height < 50)
						height = 50;
					RECT wRect;
					SetRect (&wRect, 0, 0, width, height);
					AdjustWindowRectEx (&wRect, GetWindowLong (hwnd, GWL_STYLE), FALSE, GetWindowLong (hwnd, GWL_EXSTYLE));
					width = wRect.right - wRect.left;
					height = wRect.bottom - wRect.top;
					SetWindowPos (hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
				}
			}
		}
		break;
	case WM_TIMER :
		effect = (AEffect*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
		if(effect)
			effect->dispatcher (effect, effEditIdle, 0, 0, 0, 0);
		break;
	case WM_CLOSE :
		{
			effect = (AEffect*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
			KillTimer (hwnd, 1);
			if(effect)
			{
				effect->dispatcher (effect, effEditClose, 0, 0, 0, 0);
			}

			EndDialog (hwnd, IDOK);
		}
		break;
	}

	return 0;
}

static long __cdecl audioMaster(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
	switch (opcode)
	{
	case audioMasterVersion:
		return 2300;

	case audioMasterCurrentId:
		if (effect) return effect->uniqueID;
		break;

	case audioMasterGetVendorString:
		strncpy((char *)ptr, "Chris Moeller", 64);
		//strncpy((char *)ptr, "YAMAHA", 64);
		break;

	case audioMasterGetProductString:
		strncpy((char *)ptr, "VSTi Host Bridge", 64);
		//strncpy((char *)ptr, "SOL/SQ01", 64);
		break;

	case audioMasterGetVendorVersion:
		return 1337; // uhuhuhuhu
		//return 0;

	case audioMasterGetLanguage:
		return kVstLangEnglish;

	case audioMasterWillReplaceOrAccumulate:
		return 1;

	case audioMasterNeedIdle:
		need_idle = true;
		return 1;
	}

	return 0;
}

int CALLBACK _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	int argc = 0;
	LPWSTR * argv = CommandLineToArgvW( GetCommandLineW(), &argc );

	if ( argc != 3 ) return 1;

	wchar_t * end_char = 0;
	unsigned in_sum = wcstoul( argv[ 2 ], &end_char, 16 );
	if ( end_char == argv[ 2 ] || *end_char ) return 1;

	unsigned test_sum = 0;
	end_char = argv[ 1 ];
	while ( *end_char )
	{
		test_sum += (TCHAR)( *end_char++ * 820109 );
	}

	if ( test_sum != in_sum ) return 1;

	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);

	unsigned code = 0;

	HMODULE hDll = NULL;
	main_func pMain = NULL;
	AEffect * pEffect[2] = {0};

	std::vector<uint8_t> blState;

	uint32_t sample_rate = 44100;

	std::vector<uint8_t> chunk;
	std::vector<float> sample_buffer;

	hDll = LoadLibraryW( argv[ 1 ] );
	if ( !hDll )
	{
		code = 1;
		goto exit;
	}

	pMain = (main_func) GetProcAddress( hDll, "main" );
	if ( !pMain )
	{
		code = 2;
		goto exit;
	}

	pEffect[ 0 ] = pMain( &audioMaster );
	if ( !pEffect[ 0 ] )
	{
		code = 3;
		goto exit;
	}

	uint32_t max_num_outputs = min( pEffect[ 0 ]->numOutputs, 2 );

	pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effOpen, 0, 0, 0, 0 );

	if ( pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effGetPlugCategory, 0, 0, 0, 0 ) != kPlugCategSynth ||
		pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effCanDo, 0, 0, "sendVstMidiEvent", 0 ) == 0 )
	{
		code = 4;
		goto exit;
	}

	{
		char vendor_string[65] = { 0 };
		char product_string[65] = { 0 };
		uint32_t vendor_string_length;
		uint32_t product_string_length;
		uint32_t vendor_version;
		uint32_t unique_id;

		pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effGetVendorString, 0, 0, &vendor_string, 0 );
		pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effGetProductString, 0, 0, &product_string, 0 );
		vendor_string_length = strlen( vendor_string );
		product_string_length = strlen( product_string );
		vendor_version = pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effGetVendorVersion, 0, 0, 0, 0 );
		unique_id = pEffect[ 0 ]->uniqueID;

		put_code( 0 );
		put_code( vendor_string_length );
		put_code( product_string_length );
		put_code( vendor_version );
		put_code( unique_id );
		put_code( max_num_outputs );
		if ( vendor_string_length ) put_bytes( vendor_string, vendor_string_length );
		if ( product_string_length ) put_bytes( product_string, product_string_length );
	}

	float     ** float_list_in;
	float     ** float_list_out;
	float      * float_null;
	float      * float_out;

	for (;;)
	{
		uint32_t command = get_code();
		if ( !command ) break;

		switch ( command )
		{
		case 1: // Get Chunk
			getChunk( pEffect[ 0 ], chunk );

			put_code( 0 );
			put_code( chunk.size() );
			put_bytes( chunk.data(), chunk.size() );
			break;

		case 2: // Set Chunk
			{
				uint32_t size = get_code();
				chunk.resize( size );
				if ( size ) get_bytes( chunk.data(), size );

				setChunk( pEffect[ 0 ], chunk );
				setChunk( pEffect[ 1 ], chunk );

				put_code( 0 );
			}
			break;

		case 3: // Has Editor
			{
				uint32_t has_editor = ( pEffect[ 0 ]->flags & effFlagsHasEditor ) ? 1 : 0;

				put_code( 0 );
				put_code( has_editor );
			}
			break;

		case 4: // Display Editor Modal
			{
				if ( pEffect[ 0 ]->flags & effFlagsHasEditor )
				{
					MyDLGTEMPLATE t;
					t.style = WS_POPUPWINDOW | WS_DLGFRAME | DS_MODALFRAME | DS_CENTER;
					DialogBoxIndirectParam ( 0, &t, GetDesktopWindow(), (DLGPROC)EditorProc, (LPARAM)( pEffect[ 0 ] ) );
					getChunk( pEffect[ 0 ], chunk );
					setChunk( pEffect[ 1 ], chunk );
				}

				put_code( 0 );
			}
			break;

		case 5: // Set Sample Rate
			{
				uint32_t size = get_code();
				if ( size != sizeof(sample_rate) )
				{
					code = 5;
					goto exit;
				}

				sample_rate = get_code();

				put_code( 0 );
			}
			break;

		case 6: // Reset
			{
				if ( pEffect[ 1 ] )
				{
					if ( blState.size() ) pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effStopProcess, 0, 0, 0, 0 );
					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effClose, 0, 0, 0, 0 );
					pEffect[ 1 ] = NULL;
				}
				if ( blState.size() ) pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effStopProcess, 0, 0, 0, 0 );
				pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effClose, 0, 0, 0, 0 );

				blState.resize( 0 );

				freeChain();

				pEffect[ 0 ] = pMain( &audioMaster );
				if ( !pEffect[ 0 ] )
				{
					code = 3;
					goto exit;
				}
				pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effOpen, 0, 0, 0, 0 );
				setChunk( pEffect[ 0 ], chunk );

				put_code( 0 );
			}
			break;

		case 7: // Send MIDI Event
			{
				myVstEvent * ev = ( myVstEvent * ) calloc( sizeof( myVstEvent ), 1 );
				if ( evTail ) evTail->next = ev;
				evTail = ev;
				if ( !evChain ) evChain = ev;

				uint32_t b = get_code();

				ev->port = (b & 0x7F000000) ? 1 : 0;
				ev->ev.midiEvent.type = kVstMidiType;
				ev->ev.midiEvent.byteSize = sizeof(ev->ev.midiEvent);
				memcpy(&ev->ev.midiEvent.midiData, &b, 3);

				put_code( 0 );
			}
			break;

		case 8: // Send System Exclusive Event
			{
				myVstEvent * ev = ( myVstEvent * ) calloc( sizeof( myVstEvent ), 1 );
				if ( evTail ) evTail->next = ev;
				if ( !evChain ) evChain = ev;

				uint32_t size = get_code();

				ev->ev.sysexEvent.type = kVstSysExType;
				ev->ev.sysexEvent.byteSize = sizeof(ev->ev.sysexEvent);
				ev->ev.sysexEvent.dumpBytes = size;
				ev->ev.sysexEvent.sysexDump = (char*) malloc( size );

				get_bytes( ev->ev.sysexEvent.sysexDump, size );

				evTail = ( myVstEvent * ) calloc( sizeof( myVstEvent ), 1 );
				ev->next = evTail;
				evTail->port = 1;
				evTail->ev.sysexEvent = ev->ev.sysexEvent;
				evTail->ev.sysexEvent.sysexDump = (char*) malloc( size );
				memcpy( evTail->ev.sysexEvent.sysexDump, ev->ev.sysexEvent.sysexDump, size );

				put_code( 0 );
			}
			break;

		case 9: // Render Samples
			{
				if ( !pEffect[ 1 ] )
				{
					pEffect[ 1 ] = pMain( &audioMaster );
					if ( !pEffect[ 1 ] )
					{
						code = 6;
						goto exit;
					}
					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effOpen, 0, 0, 0, 0 );
					setChunk( pEffect[ 1 ], chunk );
				}

				if ( !blState.size() )
				{
					pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effSetSampleRate, 0, 0, 0, float(sample_rate) );
					pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effSetBlockSize, 0, BUFFER_SIZE, 0, 0 );
					pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effMainsChanged, 0, 1, 0, 0 );
					pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effStartProcess, 0, 0, 0, 0 );

					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effSetSampleRate, 0, 0, 0, float(sample_rate) );
					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effSetBlockSize, 0, BUFFER_SIZE, 0, 0 );
					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effMainsChanged, 0, 1, 0, 0 );
					pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effStartProcess, 0, 0, 0, 0 );

					size_t buffer_size = sizeof(float*) * ( pEffect[ 0 ]->numInputs + pEffect[ 0 ]->numOutputs * 2 ); // float lists
					buffer_size += sizeof(float) * BUFFER_SIZE;                                // null input
					buffer_size += sizeof(float) * BUFFER_SIZE * pEffect[ 0 ]->numOutputs * 2;          // outputs

					blState.resize( buffer_size );

					float_list_in  = (float**) blState.data();
					float_list_out = float_list_in + pEffect[ 0 ]->numInputs;
					float_null     = (float*) ( float_list_out + pEffect[ 0 ]->numOutputs * 2 );
					float_out      = float_null + BUFFER_SIZE;

					for ( unsigned i = 0; i < pEffect[ 0 ]->numInputs; ++i )      float_list_in [ i ] = float_null;
					for ( unsigned i = 0; i < pEffect[ 0 ]->numOutputs * 2; ++i ) float_list_out[ i ] = float_out + BUFFER_SIZE * i;

					memset( float_null, 0, sizeof(float) * BUFFER_SIZE );

					sample_buffer.resize( BUFFER_SIZE * max_num_outputs );
				}

				if ( need_idle ) need_idle = pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effIdle, 0, 0, 0, 0 ) || pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effIdle, 0, 0, 0, 0 );

				VstEvents * events[ 2 ] = {0};

				if ( evChain )
				{
					unsigned event_count[ 2 ] = {0};
					myVstEvent * ev = evChain;
					while ( ev )
					{
						event_count[ ev->port ]++;
						ev = ev->next;
					}

					if ( event_count[ 0 ] )
					{
						events[ 0 ] = ( VstEvents * ) malloc( sizeof(long) + sizeof(long) + sizeof(VstEvent*) * event_count[ 0 ] );

						events[ 0 ]->numEvents = event_count[ 0 ];
						events[ 0 ]->reserved = 0;

						ev = evChain;

						for ( unsigned i = 0; ev; )
						{
							if ( !ev->port ) events[ 0 ]->events[ i++ ] = (VstEvent*) &ev->ev;
							ev = ev->next;
						}

						pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effProcessEvents, 0, 0, events[ 0 ], 0 );
					}

					if ( event_count[ 1 ] )
					{
						events[ 1 ] = ( VstEvents * ) malloc( sizeof(long) + sizeof(long) + sizeof(VstEvent*) * event_count[ 1 ] );

						events[ 1 ]->numEvents = event_count[ 1 ];
						events[ 1 ]->reserved = 0;

						ev = evChain;

						for ( unsigned i = 0; ev; )
						{
							if ( ev->port ) events[ 1 ]->events[ i++ ] = (VstEvent*) &ev->ev;
							ev = ev->next;
						}

						pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effProcessEvents, 0, 0, events[ 1 ], 0 );
					}
				}

				uint32_t count = get_code();

				put_code( 0 );

				while( count )
				{
					unsigned count_to_do = min( count, BUFFER_SIZE );
					unsigned num_outputs = pEffect[ 0 ]->numOutputs;

					pEffect[ 0 ]->processReplacing( pEffect[ 0 ], float_list_in, float_list_out, count_to_do );
					pEffect[ 1 ]->processReplacing( pEffect[ 1 ], float_list_in, float_list_out + num_outputs, count_to_do );

					float * out = sample_buffer.data();

					if ( max_num_outputs == 2 )
					{
						for ( unsigned i = 0; i < count_to_do; ++i )
						{
							float sample = ( float_out[ i ] + float_out[ i + BUFFER_SIZE * num_outputs ] );
							out[ 0 ] = sample;
							sample = ( float_out[ i + BUFFER_SIZE ] + float_out[ i + BUFFER_SIZE + BUFFER_SIZE * num_outputs ] );
							out[ 1 ] = sample;
							out += 2;
						}
					}
					else
					{
						for ( unsigned i = 0; i < count_to_do; ++i )
						{
							float sample = ( float_out[ i ] + float_out[ i + BUFFER_SIZE * num_outputs ] );
							out[ 0 ] = sample;
							out++;
						}
					}

					put_bytes( sample_buffer.data(), sizeof(float) * count_to_do * max_num_outputs );

					count -= count_to_do;
				}

				if ( events[ 0 ] ) free( events[ 0 ] );
				if ( events[ 1 ] ) free( events[ 1 ] );

				freeChain();
			}
			break;

		default:
			code = 6;
			goto exit;
			break;
		}
	}

exit:
	if ( pEffect[ 1 ] )
	{
		if ( blState.size() ) pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effStopProcess, 0, 0, 0, 0 );
		pEffect[ 1 ]->dispatcher( pEffect[ 1 ], effClose, 0, 0, 0, 0 );
	}
	if ( pEffect[ 0 ] )
	{
		if ( blState.size() ) pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effStopProcess, 0, 0, 0, 0 );
		pEffect[ 0 ]->dispatcher( pEffect[ 0 ], effClose, 0, 0, 0, 0 );
	}
	freeChain();
	if ( hDll ) FreeLibrary( hDll );

	put_code( code );
	return code ? 1 : 0;
}

