#include "stdafx.h"

SCCore::SCCore()
{
    handle = 0;

    TG_initialize = 0;
    //TG_terminate = 0;
    TG_activate = 0;
    // TG_deactivate = 0;
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
        ::FreeLibrary((HMODULE) handle);
        handle = 0;
    }
}

bool SCCore::Load(const wchar_t * path)
{
    handle = (void *) LoadLibraryW(path);

    if (handle)
    {
        *(void **) &TG_initialize = (void *) GetProcAddress((HMODULE) handle, "TG_initialize");
        //*(void**)&TG_terminate = (void*)GetProcAddress((HMODULE)handle, "TG_terminate");
        *(void **) &TG_activate = (void *) GetProcAddress((HMODULE) handle, "TG_activate");
        //*(void**)&TG_deactivate = (void*)GetProcAddress((HMODULE)handle, "TG_deactivate");
        *(void **) &TG_setSampleRate = (void *) GetProcAddress((HMODULE) handle, "TG_setSampleRate");
        *(void **) &TG_setMaxBlockSize = (void *) GetProcAddress((HMODULE) handle, "TG_setMaxBlockSize");
        *(void **) &TG_flushMidi = (void *) GetProcAddress((HMODULE) handle, "TG_flushMidi");
        *(void **) &TG_setInterruptThreadIdAtThisTime = (void *) GetProcAddress((HMODULE) handle, "TG_setInterruptThreadIdAtThisTime");
        //*(void**)&TG_PMidiIn = (void*)GetProcAddress((HMODULE)handle, "TG_PMidiIn");
        *(void **) &TG_ShortMidiIn = (void *) GetProcAddress((HMODULE) handle, "TG_ShortMidiIn");
        *(void **) &TG_LongMidiIn = (void *) GetProcAddress((HMODULE) handle, "TG_LongMidiIn");
        //*(void**)&TG_isFatalError = (void*)GetProcAddress((HMODULE)handle, "TG_isFatalError");
        //*(void**)&TG_getErrorStrings = (void*)GetProcAddress((HMODULE)handle, "TG_getErrorStrings");
        *(void **) &TG_XPgetCurTotalRunningVoices = (void *) GetProcAddress((HMODULE) handle, "TG_XPgetCurTotalRunningVoices");
        //*(void**)&TG_XPsetSystemConfig = (void*)GetProcAddress((HMODULE)handle, "TG_XPsetSystemConfig");
        //*(void**)&TG_XPgetCurSystemConfig = (void*)GetProcAddress((HMODULE)handle, "TG_XPgetCurSystemConfig");
        *(void **) &TG_Process = (void *) GetProcAddress((HMODULE) handle, "TG_Process");

        if (TG_initialize && /*TG_terminate &&*/ TG_activate && /*TG_deactivate &&*/
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
            // TG_deactivate = 0;
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

            ::FreeLibrary((HMODULE) handle);
            handle = 0;
        }
    }

    return false;
}
