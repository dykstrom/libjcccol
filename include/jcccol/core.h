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
int64_t millis(void);

#ifdef __cplusplus
}
#endif

#endif /* JCCCOL_CORE_H */
