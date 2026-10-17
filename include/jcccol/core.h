/**
 * core.h - Core functionality for the COL Standard Library
 *
 * Provides essential utilities and time functions.
 */

#ifndef JCCCOL_CORE_H
#define JCCCOL_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the number of milliseconds since the Unix epoch (January 1, 1970 00:00:00 UTC).
 *
 * @return Number of milliseconds since epoch
 */
int64_t col_millis(void);

/**
 * Returns a pseudo-random number in the range [0, 1).
 *
 * The generator is xoshiro256**, so a given seed produces the same
 * sequence on every platform. If col_randomize_i64 has not been called,
 * the first call seeds the generator from col_millis(). Not thread-safe.
 *
 * @return A double that is at least 0.0 and less than 1.0.
 */
double col_rnd(void);

/**
 * Seeds the generator behind col_rnd. Two runs that use the same seed get
 * the same sequence of numbers.
 *
 * @param seed Any value, including 0 and negative values.
 */
void col_randomize_i64(int64_t seed);

/**
 * Pauses the program for the given number of milliseconds.
 *
 * @param ms Milliseconds to sleep. Zero or negative returns at once.
 */
void col_sleep_i64(int64_t ms);

#ifdef __cplusplus
}
#endif

#endif /* JCCCOL_CORE_H */
