
/** $VER: Log.h (2025.07.26) **/

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

    virtual ILog & SetLevel(LogLevel level) noexcept = 0;
    virtual LogLevel GetLevel() const noexcept = 0;

    virtual ILog & AtFatal() noexcept = 0;
    virtual ILog & AtError() noexcept = 0;
    virtual ILog & AtWarn() noexcept = 0;
    virtual ILog & AtInfo() noexcept = 0;
    virtual ILog & AtDebug() noexcept = 0;
    virtual ILog & AtTrace() noexcept = 0;
    virtual ILog & Write(const char * format, ... ) noexcept = 0;
};

extern ILog & Log;
