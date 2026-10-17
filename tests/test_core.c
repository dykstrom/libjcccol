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

/* Test: col_millis returns a positive value */
TEST(col_millis_returns_positive) {
    int64_t ms = col_millis();
    ASSERT(ms > 0);
    return 0;
}

/* Test: col_millis returns a reasonable timestamp (after year 2000) */
TEST(col_millis_reasonable_timestamp) {
    int64_t ms = col_millis();
    /* January 1, 2000 00:00:00 UTC = 946684800000 milliseconds */
    int64_t year_2000_ms = 946684800000LL;
    ASSERT_MSG(ms > year_2000_ms, "Timestamp should be after year 2000");
    return 0;
}

/* Test: col_millis returns a timestamp before year 2100 */
TEST(col_millis_before_2100) {
    int64_t ms = col_millis();
    /* January 1, 2100 00:00:00 UTC = 4102444800000 milliseconds */
    int64_t year_2100_ms = 4102444800000LL;
    ASSERT_MSG(ms < year_2100_ms, "Timestamp should be before year 2100");
    return 0;
}

/* Test: col_millis advances strictly over a real sleep */
TEST(col_millis_increases) {
    int64_t ms1 = col_millis();
    test_sleep_ms(20);
    int64_t ms2 = col_millis();
    ASSERT_MSG(ms2 > ms1, "Second timestamp should be strictly greater after sleep");
    return 0;
}

/* Test: col_millis returns milliseconds, not seconds or microseconds.
 * After a ~50ms sleep, the delta must be at least ~50 (lower bound proves
 * we're in millisecond units — a seconds-based clock would still read 0
 * and a microseconds-based clock would read 50000+). The upper bound is
 * a sanity check against the timer being wildly off. */
TEST(col_millis_in_ms_units) {
    int64_t ms1 = col_millis();
    test_sleep_ms(50);
    int64_t ms2 = col_millis();
    int64_t diff = ms2 - ms1;
    ASSERT_MSG(diff >= 45 && diff < 1000, "Time difference should be ~50ms");
    return 0;
}

/* ---- Random numbers ---- */

/* Runs before any col_randomize_i64 call, so it also covers the first
 * call seeding the generator from the clock. */
TEST(col_rnd_in_range_without_seed) {
    int i;
    for (i = 0; i < 10000; i++) {
        double r = col_rnd();
        ASSERT_MSG(r >= 0.0 && r < 1.0, "col_rnd must stay in [0, 1)");
    }
    return 0;
}

/* The first three values for seed 42. Written as hex floats so the
 * comparison is exact, and checked against an independent Python version
 * of splitmix64 + xoshiro256**. Matching here on every CI platform is what
 * shows the sequence does not depend on the OS. */
TEST(col_rnd_seed_42_sequence) {
    col_randomize_i64(42);
    ASSERT(col_rnd() == 0x1.5780b2e0c2ecp-4);
    ASSERT(col_rnd() == 0x1.84136619b444ep-2);
    ASSERT(col_rnd() == 0x1.5c2ea66473c93p-1);
    return 0;
}

TEST(col_randomize_repeats_and_differs) {
    double first[5];
    double r43;
    int i;

    col_randomize_i64(42);
    for (i = 0; i < 5; i++) {
        first[i] = col_rnd();
    }
    col_randomize_i64(42);
    for (i = 0; i < 5; i++) {
        ASSERT_MSG(col_rnd() == first[i], "same seed must repeat the sequence");
    }

    col_randomize_i64(43);
    r43 = col_rnd();
    ASSERT_MSG(r43 != first[0], "a different seed must give a different value");
    return 0;
}

/* Zero and negative seeds are valid seeds, not special cases. */
TEST(col_randomize_zero_and_negative) {
    double r;

    col_randomize_i64(0);
    r = col_rnd();
    ASSERT(r >= 0.0 && r < 1.0);
    col_randomize_i64(-1);
    r = col_rnd();
    ASSERT(r >= 0.0 && r < 1.0);
    return 0;
}

/* ---- Sleeping ---- */

TEST(col_sleep_waits) {
    int64_t ms1 = col_millis();
    col_sleep_i64(50);
    int64_t diff = col_millis() - ms1;
    ASSERT_MSG(diff >= 45 && diff < 1000, "col_sleep_i64(50) should take ~50ms");
    return 0;
}

TEST(col_sleep_non_positive_returns_at_once) {
    int64_t ms1 = col_millis();
    col_sleep_i64(0);
    col_sleep_i64(-5);
    ASSERT_MSG(col_millis() - ms1 < 20, "non-positive sleeps should not wait");
    return 0;
}

int main(void) {
    int total = 0;
    int passed = 0;
    int failed = 0;

    printf("\nRunning core library tests...\n");
    printf("==============================\n\n");

    RUN_TEST(col_millis_returns_positive);
    RUN_TEST(col_millis_reasonable_timestamp);
    RUN_TEST(col_millis_before_2100);
    RUN_TEST(col_millis_increases);
    RUN_TEST(col_millis_in_ms_units);
    RUN_TEST(col_rnd_in_range_without_seed);
    RUN_TEST(col_rnd_seed_42_sequence);
    RUN_TEST(col_randomize_repeats_and_differs);
    RUN_TEST(col_randomize_zero_and_negative);
    RUN_TEST(col_sleep_waits);
    RUN_TEST(col_sleep_non_positive_returns_at_once);

    printf("\n==============================\n");
    printf("Results: %d/%d tests passed", passed, total);
    if (failed > 0) {
        printf(", %d FAILED", failed);
    }
    printf("\n\n");

    return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
