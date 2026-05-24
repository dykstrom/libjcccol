/**
 * test_framework.h - Simple test framework for libjcccol
 *
 * Provides macros for defining and running tests with assertions.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* Request POSIX.1-2008 visibility so nanosleep() is declared in <time.h>
 * under -std=c11 on glibc. Same rationale as src/core.c. Must be defined
 * before any system header is included. */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <time.h>
#endif

/**
 * Define a test function.
 * Usage: TEST(test_name) { ... }
 */
#define TEST(name) static int test_##name(void)

/**
 * Run a test and track results.
 * Updates the passed, failed, and total counters.
 */
#define RUN_TEST(name) do { \
    printf("  Running test_%s... ", #name); \
    if (test_##name() == 0) { \
        printf("PASSED\n"); \
        passed++; \
    } else { \
        printf("FAILED\n"); \
        failed++; \
    } \
    total++; \
} while(0)

/**
 * Assert that a condition is true.
 * If false, prints the condition and returns failure.
 */
#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n    Assertion failed: %s\n", #condition); \
        return 1; \
    } \
} while(0)

/**
 * Assert that a condition is true with a custom message.
 * If false, prints both the condition and the message, then returns failure.
 */
#define ASSERT_MSG(condition, msg) do { \
    if (!(condition)) { \
        printf("\n    Assertion failed: %s\n    %s\n", #condition, msg); \
        return 1; \
    } \
} while(0)

/**
 * Sleep for the given number of milliseconds. Platform-conditional:
 * Sleep() on Windows, nanosleep() on POSIX. Lets tests advance wall-clock
 * time without relying on busy-loops that the compiler might optimize.
 */
static inline void test_sleep_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

#endif /* TEST_FRAMEWORK_H */
