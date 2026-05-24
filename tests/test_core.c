/**
 * test_core.c - Tests for core COL Standard Library functions
 */

/* Request POSIX.1-2008 visibility so nanosleep() (used via the
 * test_sleep_ms helper in test_framework.h) is declared in <time.h>
 * under -std=c11 on glibc. Must precede every system header include. */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdint.h>
#include "jcccol.h"
#include "test_framework.h"

/* Test: millis returns a positive value */
TEST(millis_returns_positive) {
    int64_t ms = millis();
    ASSERT(ms > 0);
    return 0;
}

/* Test: millis returns a reasonable timestamp (after year 2000) */
TEST(millis_reasonable_timestamp) {
    int64_t ms = millis();
    /* January 1, 2000 00:00:00 UTC = 946684800000 milliseconds */
    int64_t year_2000_ms = 946684800000LL;
    ASSERT_MSG(ms > year_2000_ms, "Timestamp should be after year 2000");
    return 0;
}

/* Test: millis returns a timestamp before year 2100 */
TEST(millis_before_2100) {
    int64_t ms = millis();
    /* January 1, 2100 00:00:00 UTC = 4102444800000 milliseconds */
    int64_t year_2100_ms = 4102444800000LL;
    ASSERT_MSG(ms < year_2100_ms, "Timestamp should be before year 2100");
    return 0;
}

/* Test: millis advances strictly over a real sleep */
TEST(millis_increases) {
    int64_t ms1 = millis();
    test_sleep_ms(20);
    int64_t ms2 = millis();
    ASSERT_MSG(ms2 > ms1, "Second timestamp should be strictly greater after sleep");
    return 0;
}

/* Test: millis returns milliseconds, not seconds or microseconds.
 * After a ~50ms sleep, the delta must be at least ~50 (lower bound proves
 * we're in millisecond units — a seconds-based clock would still read 0
 * and a microseconds-based clock would read 50000+). The upper bound is
 * a sanity check against the timer being wildly off. */
TEST(millis_in_ms_units) {
    int64_t ms1 = millis();
    test_sleep_ms(50);
    int64_t ms2 = millis();
    int64_t diff = ms2 - ms1;
    ASSERT_MSG(diff >= 45 && diff < 1000, "Time difference should be ~50ms");
    return 0;
}

int main(void) {
    int total = 0;
    int passed = 0;
    int failed = 0;

    printf("\nRunning core library tests...\n");
    printf("==============================\n\n");

    RUN_TEST(millis_returns_positive);
    RUN_TEST(millis_reasonable_timestamp);
    RUN_TEST(millis_before_2100);
    RUN_TEST(millis_increases);
    RUN_TEST(millis_in_ms_units);

    printf("\n==============================\n");
    printf("Results: %d/%d tests passed", passed, total);
    if (failed > 0) {
        printf(", %d FAILED", failed);
    }
    printf("\n\n");

    return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
