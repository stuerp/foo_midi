#include "SCPlayer.h"

#include <vector>

#include <foobar2000.h>

// It's a secret to everybody!

char g_sc_name[] = { 'F','P','P','b','e','r','.','q','y','y',0 };

static struct init_sc_name
{
	init_sc_name()
	{
		char * p;
		for (p = g_sc_name; *p; ++p)
		{
			if (*p >= 'A' && *p <= 'Z') *p = (((*p - 'A') + 13) % 26) + 'A';
			else if (*p >= 'a' && *p <= 'z') *p = (((*p - 'a') + 13) % 26) + 'a';
		}
	}
} g_init_sc_name;

static uint16_t getwordle(uint8_t *pData)
{
	return (uint16_t)(pData[0] | (((uint16_t)pData[1]) << 8));
}

static uint32_t getdwordle(uint8_t *pData)
{
	return pData[0] | (((uint32_t)pData[1]) << 8) | (((uint32_t)pData[2]) << 16) | (((uint32_t)pData[3]) << 24);
}

unsigned SCPlayer::test_plugin_platform() {
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
		filesystem::g_open(f, plugin.c_str(), filesystem::open_mode_read, m_abort);

		f->read_object(peheader, iMZHeaderSize, m_abort);
		if (getwordle(peheader) != 0x5A4D) return 0;
		dwOffsetPE = getdwordle(peheader + 0x3c);
		f->seek(dwOffsetPE, m_abort);
		f->read_object(peheader, iPEHeaderSize, m_abort);
		if (getdwordle(peheader) != 0x00004550) return 0;
		switch (getwordle(peheader + 4)) {
		case 0x014C: return 32;
		case 0x8664: return 64;
		}
	}
	catch (...) {}

	return 0;
}

bool SCPlayer::LoadCore(const char * path)
{
	if (path != sPlugin) sPlugin = path;

	uPluginPlatform = test_plugin_platform();

	if (!uPluginPlatform) return false;

	bool rval = process_create(0);
	if (rval) rval = process_create(1);
	if (rval) rval = process_create(2);

	return rval;
}

static bool create_pipe_name(pfc::string_base & out)
{
	GUID guid;
	if (FAILED(CoCreateGuid(&guid))) return false;

	out = "\\\\.\\pipe\\";
	out += pfc::print_guid(guid);

	return true;
}

bool SCPlayer::process_create(uint32_t port)
{
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!iInitialized)
	{
		if (FAILED(CoInitialize(NULL))) return false;
	}
	iInitialized++;

	hReadEvent[port] = CreateEvent(NULL, TRUE, FALSE, NULL);

	pfc::string8 pipe_name_in, pipe_name_out;
	if (!create_pipe_name(pipe_name_in) || !create_pipe_name(pipe_name_out))
	{
		process_terminate(port);
		return false;
	}

	pfc::stringcvt::string_os_from_utf8 pipe_name_in_os(pipe_name_in), pipe_name_out_os(pipe_name_out);

	HANDLE hPipe = CreateNamedPipe(pipe_name_in_os, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		process_terminate(port);
		return false;
	}
	hChildStd_IN_Rd[port] = CreateFile(pipe_name_in_os, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);
	DuplicateHandle(GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_IN_Wr[port], 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hPipe);

	hPipe = CreateNamedPipe(pipe_name_out_os, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		process_terminate(port);
		return false;
	}
	hChildStd_OUT_Wr[port] = CreateFile(pipe_name_out_os, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);
	DuplicateHandle(GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_OUT_Rd[port], 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hPipe);

	std::string szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	size_t slash = szCmdLine.find_last_of('\\');
	if (slash != std::string::npos)
		szCmdLine.erase(szCmdLine.begin() + slash + 1, szCmdLine.end());
	szCmdLine += (uPluginPlatform == 64) ? "scpipe64.exe" : "scpipe32.exe";
	szCmdLine += "\" \"";
	szCmdLine += sPlugin;
	szCmdLine += "\"";

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo = { 0 };

	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.hStdInput = hChildStd_IN_Rd[port];
	siStartInfo.hStdOutput = hChildStd_OUT_Wr[port];
	siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	//siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES; // | STARTF_USESHOWWINDOW;

	if (!CreateProcess(NULL, (LPTSTR)(LPCTSTR)pfc::stringcvt::string_os_from_utf8(szCmdLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo))
	{
		process_terminate(port);
		return false;
	}

	// Close remote handles so pipes will break when process terminates
	CloseHandle(hChildStd_OUT_Wr[port]);
	CloseHandle(hChildStd_IN_Rd[port]);
	hChildStd_OUT_Wr[port] = NULL;
	hChildStd_IN_Rd[port] = NULL;

	hProcess[port] = piProcInfo.hProcess;
	hThread[port] = piProcInfo.hThread;

#ifdef NDEBUG
	SetPriorityClass(hProcess[port], GetPriorityClass(GetCurrentProcess()));
	SetThreadPriority(hThread[port], GetThreadPriority(GetCurrentThread()));
#endif

	uint32_t code = process_read_code(port);

	if (code != 0)
	{
		process_terminate(port);
		return false;
	}

	return true;
}

void SCPlayer::process_terminate(uint32_t port)
{
	if (bTerminating[port]) return;

	bTerminating[port] = true;

	if (hProcess[port])
	{
		process_write_code(port, 0);
		WaitForSingleObject(hProcess[port], 5000);
		TerminateProcess(hProcess[port], 0);
		CloseHandle(hThread[port]);
		CloseHandle(hProcess[port]);
	}
	if (hChildStd_IN_Rd[port]) CloseHandle(hChildStd_IN_Rd[port]);
	if (hChildStd_IN_Wr[port]) CloseHandle(hChildStd_IN_Wr[port]);
	if (hChildStd_OUT_Rd[port]) CloseHandle(hChildStd_OUT_Rd[port]);
	if (hChildStd_OUT_Wr[port]) CloseHandle(hChildStd_OUT_Wr[port]);
	if (hReadEvent[port]) CloseHandle(hReadEvent[port]);
	if (--iInitialized == 0) CoUninitialize();
	iInitialized = 0;
	bTerminating[port] = false;
	hProcess[port] = NULL;
	hThread[port] = NULL;
	hReadEvent[port] = NULL;
	hChildStd_IN_Rd[port] = NULL;
	hChildStd_IN_Wr[port] = NULL;
	hChildStd_OUT_Rd[port] = NULL;
	hChildStd_OUT_Wr[port] = NULL;
}

bool SCPlayer::process_running(uint32_t port)
{
	if (hProcess[port] && WaitForSingleObject(hProcess[port], 0) == WAIT_TIMEOUT) return true;
	return false;
}

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

static void ProcessPendingMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
}

uint32_t SCPlayer::process_read_bytes_pass(uint32_t port, void * out, uint32_t size)
{
	OVERLAPPED ol = {};
	ol.hEvent = hReadEvent[port];
	ResetEvent(hReadEvent);
	DWORD bytesDone;
	SetLastError(NO_ERROR);
	if (ReadFile(hChildStd_OUT_Rd[port], out, size, &bytesDone, &ol)) return bytesDone;
	if (::GetLastError() != ERROR_IO_PENDING) return 0;

	const HANDLE handles[1] = { hReadEvent[port] };
	SetLastError(NO_ERROR);
	DWORD state;
	for (;;)
	{
		state = MsgWaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE, QS_ALLEVENTS);
		if (state == WAIT_OBJECT_0 + _countof(handles)) ProcessPendingMessages();
		else break;
	}

	if (state == WAIT_OBJECT_0 && GetOverlappedResult(hChildStd_OUT_Rd[port], &ol, &bytesDone, TRUE)) return bytesDone;

#if _WIN32_WINNT >= 0x600
	CancelIoEx(hChildStd_OUT_Rd, &ol);
#else
	CancelIo(hChildStd_OUT_Rd[port]);
#endif

	return 0;
}

void SCPlayer::process_read_bytes(uint32_t port, void * out, uint32_t size)
{
	if (process_running(port) && size)
	{
		uint8_t * ptr = (uint8_t *)out;
		uint32_t done = 0;
		while (done < size)
		{
			uint32_t delta = process_read_bytes_pass(port, ptr + done, size - done);
			if (delta == 0)
			{
				memset(out, 0xFF, size);
				break;
			}
			done += delta;
		}
	}
	else memset(out, 0xFF, size);
}

uint32_t SCPlayer::process_read_code(uint32_t port)
{
	uint32_t code;
	process_read_bytes(port, &code, sizeof(code));
	return code;
}

void SCPlayer::process_write_bytes(uint32_t port, const void * in, uint32_t size)
{
	if (process_running(port))
	{
		if (size == 0) return;
		DWORD bytesWritten;
		if (!WriteFile(hChildStd_IN_Wr[port], in, size, &bytesWritten, NULL) || bytesWritten < size) process_terminate(port);
	}
}

void SCPlayer::process_write_code(uint32_t port, uint32_t code)
{
	process_write_bytes(port, &code, sizeof(code));
}

SCPlayer::SCPlayer() : MIDIPlayer(), mode(sc_default), sccore_path(0)
{
	initialized = false;
	iInitialized = 0;
	for (unsigned int i = 0; i < 3; ++i)
	{
		bTerminating[i] = false;
		hProcess[i] = NULL;
		hThread[i] = NULL;
		hReadEvent[i] = NULL;
		hChildStd_IN_Rd[i] = NULL;
		hChildStd_IN_Wr[i] = NULL;
		hChildStd_OUT_Rd[i] = NULL;
		hChildStd_OUT_Wr[i] = NULL;
	}
}

SCPlayer::~SCPlayer()
{
	shutdown();
	if (sccore_path)
		free(sccore_path);
}

void SCPlayer::set_reverb(bool enable)
{
	reverb = enable;
	reset(0);
	reset(1);
	reset(2);
}

void SCPlayer::set_sccore_path(const char *path)
{
	size_t len;
	if (sccore_path) free(sccore_path);
	len = strlen(path);
	sccore_path = (char *)malloc(len + 1);
	if (sccore_path)
		memcpy(sccore_path, path, len + 1);
}

static const uint8_t syx_reset_gm[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t syx_reset_gm2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t syx_reset_gs[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t syx_reset_xg[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool syx_equal(const uint8_t * a, const uint8_t * b)
{
	while (*a != 0xF7 && *b != 0xF7 && *a == *b)
	{
		a++; b++;
	}
	
	return *a == *b;
}

static bool syx_is_reset(const uint8_t * data)
{
	return syx_equal(data, syx_reset_gm) || syx_equal(data, syx_reset_gm2) || syx_equal(data, syx_reset_gs) || syx_equal(data, syx_reset_xg);
}

static bool syx_is_gs_limit_bank_lsb(const uint8_t * data)
{
	const uint8_t * a = data, * b = syx_gs_limit_bank_lsb;
	while (*a != 0xF7 && *b != 0xF7 && *a == *b && (a - data) < 6)
	{
		a++; b++;
	}
	if ((a - data) < 6) return false;
	unsigned char checksum = 0;
	unsigned int i;
	for (i = 5; data[i + 1] != 0xF7; ++i)
		checksum += data[i];
	checksum = (128 - checksum) & 127;
	if (i != 9 || data[9] != checksum) return false;
	if (data[7] != 1) return false;
	if (data[6] < 0x40 || data[6] > 0x4F) return false;
	return true;
}

void SCPlayer::send_sysex_simple(uint32_t port, const uint8_t * data, uint32_t size)
{
	process_write_code(port, 3);
	process_write_code(port, size);
	process_write_bytes(port, data, size);
	if (process_read_code(port) != 0)
		process_terminate(port);
}

void SCPlayer::send_sysex(uint32_t port, const uint8_t * data, uint32_t size)
{
	if (syx_is_gs_limit_bank_lsb(data) && mode != sc_default)
		return;
	send_sysex_simple(port, data, size);
	if (syx_is_reset(data) && mode != sc_default)
	{
		reset(port);
	}
}

void SCPlayer::send_gs(uint32_t port, uint8_t * data, uint32_t size)
{
	unsigned long i;
	unsigned char checksum = 0;
	for (i = 5; data[i+1] != 0xF7; ++i)
		checksum += data[i];
	checksum = (128 - checksum) & 127;
	data[i] = checksum;
	send_sysex_simple(port, data, size);
}

void SCPlayer::reset_sc(uint32_t port)
{
	unsigned int i;
	uint8_t message[11];
	
	memcpy(message, syx_gs_limit_bank_lsb, 11);

	message[7] = 1;

	switch (mode)
	{
		default: break;
			
		case sc_sc55:
			message[8] = 1;
			break;
			
		case sc_sc88:
			message[8] = 2;
			break;
			
		case sc_sc88pro:
			message[8] = 3;
			break;
			
		case sc_sc8850:
		case sc_default:
			message[8] = 4;
			break;
	}
	
	for (i = 0x41; i <= 0x49; ++i)
	{
		message[6] = i;
		send_gs(port, message, 11); junk(port, 128);
	}
	message[6] = 0x40;
	send_gs(port, message, 11); junk(port, 128);
	for (i = 0x4A; i <= 0x4F; ++i)
	{
		message[6] = i;
		send_gs(port, message, 11); junk(port, 128);
	}
}

void SCPlayer::reset(uint32_t port)
{
	if (initialized)
	{
		send_sysex_simple(port, syx_reset_xg, sizeof(syx_reset_xg)); junk(port, 1024);
		send_sysex_simple(port, syx_reset_gm2, sizeof(syx_reset_gm2)); junk(port, 1024);
		send_sysex_simple(port, syx_reset_gm, sizeof(syx_reset_gm)); junk(port, 1024);

		switch (mode)
		{
			case sc_gm:
				/*sampler[port].TG_LongMidiIn( syx_reset_gm, 0 );*/
				break;
				
			case sc_gm2:
				send_sysex_simple(port, syx_reset_gm2, sizeof(syx_reset_gm2));
				break;
				
			case sc_sc55:
			case sc_sc88:
			case sc_sc88pro:
			case sc_sc8850:
			case sc_default:
				send_sysex_simple(port, syx_reset_gs, sizeof(syx_reset_gs));
				reset_sc(port);
				break;
				
			case sc_xg:
				send_sysex_simple(port, syx_reset_xg, sizeof(syx_reset_xg));
				break;
		}

		junk(port, 1024);

		{
			unsigned int i;
			for (i = 0; i < 16; ++i)
			{
				send_command(port, 0x78B0 + i);
				send_command(port, 0x79B0 + i);
				if (mode != sc_xg || i != 9)
				{
					send_command(port, 0x20B0 + i);
					send_command(port, 0x00B0 + i);
					send_command(port, 0xC0 + i);
				}
			}
		}

		if (mode == sc_xg)
		{
			send_command(port, 0x20B9);
			send_command(port, 0x7F00B9);
			send_command(port, 0xC9);
		}

		if (!reverb)
		{
			unsigned int i;
			for (i = 0; i < 16; ++i)
			{
				send_command(port, 0x5BB0 + i);
				send_command(port, 0x5DB0 + i);
			}
		}

		junk(port, uSampleRate * 2 / 3);
	}
}

void SCPlayer::junk(uint32_t port, unsigned long count)
{
	while (count)
	{
		uint32_t do_count = (uint32_t)((unsigned long)(min(count, UINT_MAX)));
		process_write_code(port, 5);
		process_write_code(port, do_count);
		if (process_read_code(port) != 0)
		{
			process_terminate(port);
			break;
		}
		count -= do_count;
	}
}

void SCPlayer::send_command(uint32_t port, uint32_t command)
{
	process_write_code(port, 2);
	process_write_code(port, command);
	if (process_read_code(port) != 0)
		process_terminate(port);
}

void SCPlayer::set_mode(sc_mode m)
{
	mode = m;
	reset(0);
	reset(1);
	reset(2);
}

void SCPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned port = (b >> 24) & 0x7F;
		if ( port > 2 ) port = 2;
		if (!reverb && (((b & 0x7FF0) == 0x5BB0) || ((b & 0x7FF0) == 0x5DB0)))
			return;
		send_command(port, b);
	}
	else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
		std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
		send_sysex( port, data, size );
		if ( port == 0 )
		{
			send_sysex( 1, data, size );
			send_sysex( 2, data, size );
		}
	}
}

void SCPlayer::render_port(uint32_t port, float * out, uint32_t count)
{
	process_write_code(port, 4);
	process_write_code(port, count);
	if (process_read_code(port) != 0)
	{
		process_terminate(port);
		memset(out, 0, count * sizeof(float) * 2);
		return;
	}
	process_read_bytes(port, out, count * sizeof(float) * 2);
}

void SCPlayer::render(float * out, unsigned long count)
{
	memset(out, 0, count * sizeof(float) * 2);
	while (count)
	{
		unsigned long todo = count > 4096 ? 4096 : count;
		float buffer[4096 * 2];
		for (unsigned long i = 0; i < 3; ++i)
		{
			render_port(i, buffer, todo);

			for (unsigned long j = 0; j < todo; ++j)
			{
				out[j * 2 + 0] += buffer[j * 2 + 0];
				out[j * 2 + 1] += buffer[j * 2 + 1];
			}
		}
		out += todo * 2;
		count -= todo;
	}
}

void SCPlayer::shutdown()
{
	for (int i = 0; i < 3; i++)
	{
		process_terminate(i);
	}
	initialized = false;
}

bool SCPlayer::startup()
{
	pfc::string8 path;

    if (initialized) return true;

	if (!sccore_path) return false;

	path = sccore_path;
	path += "\\";
	path += g_sc_name;

	if (!LoadCore(path))
		return false;

	for (int i = 0; i < 3; i++)
	{
		process_write_code(i, 1);
		process_write_code(i, sizeof(uint32_t));
		process_write_code(i, uSampleRate);
		if (process_read_code(i) != 0)
			return false;
	}

	initialized = true;
    
    for (int i = 0; i < 3; i++)
    {
        reset(i);
    }
    
	return true;
}

bool SCPlayer::send_event_needs_time()
{
	return true;
}