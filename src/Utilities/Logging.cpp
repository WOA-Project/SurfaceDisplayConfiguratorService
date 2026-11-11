#include "pch.h"
#include "Utilities/Logging.h"

#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace {
std::mutex g_logMutex;

void
Write(const char *prefix, const char *format, va_list args)
{
    std::lock_guard<std::mutex> lock(g_logMutex);

    char message[4096]{};

    vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);

    char finalMessage[4352]{};

    sprintf_s(finalMessage, "[%s] %s\n", prefix, message);

#ifdef _DEBUG
    OutputDebugStringA(finalMessage);
#endif

    fputs(finalMessage, stdout);

    fflush(stdout);
}
} // namespace

void
LogTrace(const char *format, ...)
{
#ifdef _DEBUG

    va_list args;

    va_start(args, format);

    Write("TRACE", format, args);

    va_end(args);

#endif
}

void
LogDebug(const char *format, ...)
{
#ifdef _DEBUG

    va_list args;

    va_start(args, format);

    Write("DEBUG", format, args);

    va_end(args);

#endif
}

void
LogInfo(const char *format, ...)
{
    va_list args;

    va_start(args, format);

    Write("INFO", format, args);

    va_end(args);
}

void
LogError(const char *format, ...)
{
    va_list args;

    va_start(args, format);

    Write("ERROR", format, args);

    va_end(args);
}