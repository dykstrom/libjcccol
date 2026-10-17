/**
 * core.c - Implementation of core COL Standard Library functions
 */

/* Request POSIX.1-2008 visibility from glibc so clock_gettime(),
 * CLOCK_REALTIME and nanosleep() are declared in <time.h> under -std=c11.
 * macOS does not need this (its headers are permissive), but it is
 * harmless there and on MinGW. Must be defined before any system header
 * is included. */
#define _POSIX_C_SOURCE 200809L

#include "jcccol/core.h"

#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
    #include <time.h>
#endif

int64_t col_millis(void) {
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

/* ---- Random numbers ----
 *
 * xoshiro256** by Blackman and Vigna (https://prng.di.unimi.it/), seeded
 * through splitmix64 as its authors recommend. A self-contained generator
 * rather than libc rand(), whose RAND_MAX is only 32767 on Windows and
 * whose sequence differs between platforms. */

static uint64_t rnd_state[4];
static bool rnd_seeded = false;

static uint64_t rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static uint64_t xoshiro256ss(void) {
    uint64_t result = rotl(rnd_state[1] * 5, 7) * 9;
    uint64_t t = rnd_state[1] << 17;

    rnd_state[2] ^= rnd_state[0];
    rnd_state[3] ^= rnd_state[1];
    rnd_state[1] ^= rnd_state[2];
    rnd_state[0] ^= rnd_state[3];
    rnd_state[2] ^= t;
    rnd_state[3] = rotl(rnd_state[3], 45);

    return result;
}

void col_randomize_i64(int64_t seed) {
    /* splitmix64 never yields four zero words, the one state xoshiro
     * cannot leave. */
    uint64_t x = (uint64_t) seed;
    int i;

    for (i = 0; i < 4; i++) {
        rnd_state[i] = splitmix64(&x);
    }
    rnd_seeded = true;
}

double col_rnd(void) {
    if (!rnd_seeded) {
        col_randomize_i64(col_millis());
    }
    /* The top 53 bits fill a double's mantissa exactly, giving every
     * multiple of 2^-53 in [0, 1) with equal probability. */
    return (double) (xoshiro256ss() >> 11) * 0x1.0p-53;
}

/* ---- Sleeping ---- */

void col_sleep_i64(int64_t ms) {
    if (ms <= 0) {
        return;
    }
#ifdef _WIN32
    /* INFINITE (0xFFFFFFFF) would never return, so clamp just below it. */
    Sleep(ms >= 0xFFFFFFFFLL ? 0xFFFFFFFEUL : (DWORD) ms);
#else
    struct timespec request;
    struct timespec remaining;

    request.tv_sec = (time_t) (ms / 1000);
    request.tv_nsec = (long) (ms % 1000) * 1000000L;

    /* A signal handler can cut the sleep short; finish the rest. */
    while (nanosleep(&request, &remaining) != 0 && errno == EINTR) {
        request = remaining;
    }
#endif
}
