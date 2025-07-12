
/** $VER: Log.cpp (2025.07.12) P. Stuer - Another logger implementation **/

#include "pch.h"

#include "Log.h"

class NullLog : public ILog
{
public:
    NullLog() noexcept { };

    NullLog(const NullLog &) = delete;
    NullLog(const NullLog &&) = delete;
    NullLog & operator=(const NullLog &) = delete;
    NullLog & operator=(NullLog &&) = delete;

    virtual ~NullLog() final { };

    virtual ILog & AtFatal() override final { return *this; }
    virtual ILog & AtError() override final { return *this; }
    virtual ILog & AtWarn() override final { return *this; }
    virtual ILog & AtInfo() override final { return *this; }
    virtual ILog & AtDebug() override final { return *this; }
    virtual ILog & AtTrace() override final { return *this; }
    virtual ILog & Format(const char * format, ... ) override final { return *this; }
};

ILog & Null = *new NullLog();

class LogImpl : public ILog
{
public:
#ifdef _DEBUG
    LogImpl() noexcept : _Level(LogLevel::Debug) { }
#else
    LogImpl() noexcept : _Level(LogLevel::Info) { }
#endif

    LogImpl(const LogImpl &) = delete;
    LogImpl(const LogImpl &&) = delete;
    LogImpl & operator=(const LogImpl &) = delete;
    LogImpl & operator=(LogImpl &&) = delete;

    virtual ~LogImpl() noexcept { };

    ILog & AtFatal() override final
    {
        return (_Level >= LogLevel::Fatal) ? *this : Null;
    }

    ILog & AtError() override final
    {
        return (_Level >= LogLevel::Error) ? *this : Null;
    }

    ILog & AtWarn() override final
    {
        return (_Level >= LogLevel::Warn) ? *this : Null;
    }

    ILog & AtInfo() override final
    {
        return (_Level >= LogLevel::Info) ? *this : Null;
    }

    ILog & AtDebug() override final
    {
        return (_Level >= LogLevel::Debug) ? *this : Null;
    }

    ILog & AtTrace() override final
    {
        return (_Level >= LogLevel::Trace) ? *this : Null;
    }

    ILog & Format(const char * format, ... ) override final
    {
        char Line[1024] = { };

        va_list args;

        va_start(args, format);

        (void) ::vsnprintf(Line, sizeof(Line) - 1, format, args);
        console::print(Line);

        return *this;
    }

private:
    LogLevel _Level;
};

ILog & Log = *new LogImpl();

//::GetCurrentThreadId()
