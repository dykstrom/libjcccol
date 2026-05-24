/**
 * test_framework.h - Simple test framework for libjcccol
 *
 * Provides macros for defining and running tests with assertions.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* NOTE: tests using test_sleep_ms() must define _POSIX_C_SOURCE 200809L
 * at the top of the .c file (before any system header is included) so
 * glibc declares nanosleep() under -std=c11. Defining it here in the
 * header is too late if the caller has already included <stdlib.h> etc.
 * See tests/test_core.c for the pattern. */

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
