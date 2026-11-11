#pragma once

void
LogTrace(const char *format, ...);

void
LogDebug(const char *format, ...);

void
LogInfo(const char *format, ...);

void
LogError(const char *format, ...);

#ifdef _DEBUG

#    define TRACE(...) LogTrace(__VA_ARGS__)
#    define DEBUG_LOG(...) LogDebug(__VA_ARGS__)

#else

#    define TRACE(...)
#    define DEBUG_LOG(...)

#endif

#define INFO_LOG(...) LogInfo(__VA_ARGS__)
#define ERROR_LOG(...) LogError(__VA_ARGS__)