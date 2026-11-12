
/** $VER: Log.h (2025.10.08) **/

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

class ilog_t
{
public:
    ilog_t() noexcept { };

    ilog_t(const ilog_t &) = delete;
    ilog_t(const ilog_t &&) = delete;
    ilog_t & operator=(const ilog_t &) = delete;
    ilog_t & operator=(ilog_t &&) = delete;

    virtual ~ilog_t() noexcept { };

    virtual ilog_t & SetLevel(LogLevel level) noexcept = 0;
    virtual LogLevel GetLevel() const noexcept = 0;

    virtual ilog_t & AtFatal() noexcept = 0;
    virtual ilog_t & AtError() noexcept = 0;
    virtual ilog_t & AtWarn() noexcept = 0;
    virtual ilog_t & AtInfo() noexcept = 0;
    virtual ilog_t & AtDebug() noexcept = 0;
    virtual ilog_t & AtTrace() noexcept = 0;
    virtual ilog_t & Write(const char * format, ... ) noexcept = 0;
};

extern ilog_t & Log;
