/**
 * core.c - Implementation of core COL Standard Library functions
 */

/* Request POSIX.1-2008 visibility from glibc so clock_gettime() and
 * CLOCK_REALTIME are declared in <time.h> under -std=c11. macOS does not
 * need this (its headers are permissive), but it is harmless there and on
 * MinGW. Must be defined before any system header is included. */
#define _POSIX_C_SOURCE 200809L

#include "jcccol/core.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

int64_t millis(void) {
#ifdef _WIN32
    /* Windows implementation using GetSystemTimeAsFileTime */
    FILETIME ft;
    ULARGE_INTEGER uli;

    /* Get system time as FILETIME (100-nanosecond intervals since Jan 1, 1601) */
    GetSystemTimeAsFileTime(&ft);

    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    /* Convert to milliseconds and adjust for Unix epoch */
    /* Windows epoch is Jan 1, 1601; Unix epoch is Jan 1, 1970 */
    /* Difference is 11644473600 seconds */
    return (int64_t)((uli.QuadPart / 10000ULL) - 11644473600000ULL);
#else
    /* POSIX implementation using clock_gettime(CLOCK_REALTIME).
     * Preferred over gettimeofday, which POSIX.1-2008 marks legacy. */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (int64_t)(ts.tv_sec) * 1000 + (int64_t)(ts.tv_nsec) / 1000000;
#endif
}
