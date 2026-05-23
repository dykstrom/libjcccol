/**
 * test_framework.h - Simple test framework for libjcccol
 *
 * Provides macros for defining and running tests with assertions.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

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

#endif /* TEST_FRAMEWORK_H */
