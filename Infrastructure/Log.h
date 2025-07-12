
/** $VER: Log.h (2025.07.12) **/

#pragma once

enum LogLevel
{
    Never = 0,  // Disables all logging

    Fatal,      // Very severe errors likely to cause a crash.
    Error,      // Serious issue that may allow the application to continue running.
    Warn,       // Potential problems or unexpected situations.
    Info,       // General runtime events (startup, shutdown, user actions).
    Debug,      // Detailed information useful for debugging.
    Trace,      // Very detailed information used to trace execution.

    Always      // Enables all logging
};

class ILog
{
public:
    ILog() noexcept { };

    ILog(const ILog &) = delete;
    ILog(const ILog &&) = delete;
    ILog & operator=(const ILog &) = delete;
    ILog & operator=(ILog &&) = delete;

    virtual ~ILog() noexcept { };

    virtual ILog & AtFatal() = 0;
    virtual ILog & AtError() = 0;
    virtual ILog & AtWarn() = 0;
    virtual ILog & AtInfo() = 0;
    virtual ILog & AtDebug() = 0;
    virtual ILog & AtTrace() = 0;
    virtual ILog & Format(const char * format, ... ) = 0;
};

extern ILog & Log;
