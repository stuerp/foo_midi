#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

#include "SCCore.h"

#include "../shared/shared.h"

static struct cleanup_proc
{
	cleanup_proc()
	{
		pfc::string8 temp_path;
		pfc::string8_fast temp_file;
		if (uGetTempPath(temp_path))
		{
			temp_file = temp_path;
			temp_file += "SCC*.tmp";
			uFindFile *file = uFindFirstFile(temp_file);
			while (file)
			{
				temp_file = temp_path;
				temp_file += file->GetFileName();
				uDeleteFile(temp_file);
				if (!file->FindNext()) break;
			}
		}
	}
} do_cleanup;

SCCore::SCCore()
{
	duped = false;
	path = 0;
	
	handle = 0;
	
	TG_initialize = 0;
	//TG_terminate = 0;
	TG_activate = 0;
	TG_deactivate = 0;
	TG_setSampleRate = 0;
	TG_setMaxBlockSize = 0;
	TG_flushMidi = 0;
	TG_setInterruptThreadIdAtThisTime = 0;
	// TG_PMidiIn = 0;
	TG_ShortMidiIn = 0;
	TG_LongMidiIn = 0;
	// TG_isFatalError = 0;
	// TG_getErrorStrings = 0;
	TG_XPgetCurTotalRunningVoices = 0;
	// TG_XPsetSystemConfig = 0;
	// TG_XPgetCurSystemConfig = 0;
	TG_Process = 0;
}

SCCore::~SCCore()
{
	Unload();
}

void SCCore::Unload()
{
	if (handle)
	{
		FreeLibrary((HMODULE)handle);
		handle = 0;
	}
	if (duped && path)
	{
		DeleteFileW(path);
		duped = false;
	}
	if (path)
	{
		free(path);
		path = 0;
	}
}

bool SCCore::Load(const wchar_t * _path, bool dupe)
{
	if (dupe)
	{
		wchar_t temp_path[MAX_PATH];
		DWORD length = GetTempPathW(MAX_PATH, temp_path);
		if (length > 32767 || length == 0)
			return false;
		
		path = (wchar_t *) malloc(MAX_PATH * sizeof(wchar_t));
		if (!path)
			return false;
		
		if (!GetTempFileNameW(temp_path, L"SCC", 0, path))
			return false;

		if (!CopyFileW(_path, path, FALSE))
			return false;
		
		duped = true;
	}
	else
	{
		path = (wchar_t *) malloc((wcslen(_path) + 1) * sizeof(wchar_t));
		wcscpy(path, _path);
	}
	
	handle = (void *) LoadLibraryW(path);
	if (handle)
	{
		*(void**)&TG_initialize = GetProcAddress((HMODULE)handle, "TG_initialize");
		//*(void**)&TG_terminate = GetProcAddress((HMODULE)handle, "TG_terminate");
		*(void**)&TG_activate = GetProcAddress((HMODULE)handle, "TG_activate");
		*(void**)&TG_deactivate = GetProcAddress((HMODULE)handle, "TG_deactivate");
		*(void**)&TG_setSampleRate = GetProcAddress((HMODULE)handle, "TG_setSampleRate");
		*(void**)&TG_setMaxBlockSize = GetProcAddress((HMODULE)handle, "TG_setMaxBlockSize");
		*(void**)&TG_flushMidi = GetProcAddress((HMODULE)handle, "TG_flushMidi");
		*(void**)&TG_setInterruptThreadIdAtThisTime = GetProcAddress((HMODULE)handle, "TG_setInterruptThreadIdAtThisTime");
		//*(void**)&TG_PMidiIn = GetProcAddress((HMODULE)handle, "TG_PMidiIn");
		*(void**)&TG_ShortMidiIn = GetProcAddress((HMODULE)handle, "TG_ShortMidiIn");
		*(void**)&TG_LongMidiIn = GetProcAddress((HMODULE)handle, "TG_LongMidiIn");
		//*(void**)&TG_isFatalError = GetProcAddress((HMODULE)handle, "TG_isFatalError");
		//*(void**)&TG_getErrorStrings = GetProcAddress((HMODULE)handle, "TG_getErrorStrings");
		*(void**)&TG_XPgetCurTotalRunningVoices = GetProcAddress((HMODULE)handle, "TG_XPgetCurTotalRunningVoices");
		//*(void**)&TG_XPsetSystemConfig = GetProcAddress((HMODULE)handle, "TG_XPsetSystemConfig");
		//*(void**)&TG_XPgetCurSystemConfig = GetProcAddress((HMODULE)handle, "TG_XPgetCurSystemConfig");
		*(void**)&TG_Process = GetProcAddress((HMODULE)handle, "TG_Process");
		
		if (TG_initialize && /*TG_terminate &&*/ TG_activate && TG_deactivate &&
		   TG_setSampleRate && TG_setMaxBlockSize && TG_flushMidi &&
		   TG_setInterruptThreadIdAtThisTime && /*TG_PMidiIn &&*/
		   TG_ShortMidiIn && TG_LongMidiIn && /*TG_isFatalError &&
		   TG_getErrorStrings &&*/ TG_XPgetCurTotalRunningVoices &&
		   /*TG_XPsetSystemConfig && TG_XPgetCurSystemConfig &&*/
		   TG_Process)
		{
			return true;
		}
		else
		{
			TG_initialize = 0;
			//TG_terminate = 0;
			TG_activate = 0;
			TG_deactivate = 0;
			TG_setSampleRate = 0;
			TG_setMaxBlockSize = 0;
			TG_flushMidi = 0;
			TG_setInterruptThreadIdAtThisTime = 0;
			// TG_PMidiIn = 0;
			TG_ShortMidiIn = 0;
			TG_LongMidiIn = 0;
			// TG_isFatalError = 0;
			// TG_getErrorStrings = 0;
			TG_XPgetCurTotalRunningVoices = 0;
			// TG_XPsetSystemConfig = 0;
			// TG_XPgetCurSystemConfig = 0;
			TG_Process = 0;
			
			FreeLibrary((HMODULE)handle);
			handle = 0;
		}
	}
	
	return false;
}
