/**
 * strings.h - String functions for the COL Standard Library
 *
 * A COL string is an immutable, null-terminated, UTF-8, byte-transparent
 * `char *`. Every function here operates on BYTES, never codepoints, and
 * never inspects the encoding — non-ASCII text passes through unchanged.
 * Offsets and lengths are 0-based byte counts.
 *
 * Two invariants hold across every function:
 *
 *   1. Strings are never null. Every function returning a string returns
 *      a valid pointer to a NUL-terminated block, using the empty string
 *      where there is nothing to return.
 *
 *   2. Every returned string is a FRESH malloc'd block owned by nobody.
 *      The compiler hands it to jcc_gc_register, which takes ownership
 *      and eventually frees it. A returned pointer therefore never points
 *      into static storage, a caller-supplied buffer, or a shared cache —
 *      not even for the empty string, which is its own malloc'd "".
 *
 * Arguments are never mutated. String length and equality are absent by
 * design: the compiler calls libc's strlen and strcmp directly.
 *
 * If an allocation fails, these functions print a diagnostic to stderr
 * and exit(1) rather than returning NULL, because invariant 1 is what the
 * rest of the runtime relies on.
 */

#ifndef JCCCOL_STRINGS_H
#define JCCCOL_STRINGS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Concatenates s and t.
 *
 * @param s The first string.
 * @param t The second string.
 * @return A fresh block holding s followed by t.
 */
char *col_concat_str_str(const char *s, const char *t);

/**
 * Extracts a substring of s, by byte offset. Total: out-of-range
 * arguments clamp rather than raising an error.
 *
 * A negative start yields the empty string, which makes the natural
 * composition read correctly — col_indexof_str_str returns -1 when the
 * needle is absent, and feeding that in here gives "" rather than the
 * first `length` bytes.
 *
 * @param s The source string.
 * @param start 0-based byte offset to start at. Negative, or at or past
 *              the end of s, yields the empty string.
 * @param length Number of bytes to take. Non-positive yields the empty
 *               string; a length running past the end of s is truncated.
 * @return A fresh block holding the selected bytes.
 */
char *col_substr_str_i64_i64(const char *s, int64_t start, int64_t length);

/**
 * Finds the first occurrence of needle in s.
 *
 * @param s The string to search.
 * @param needle The string to search for. An empty needle returns 0.
 * @return The 0-based byte offset of the first occurrence, or -1 if
 *         needle does not occur in s. Never an interior pointer.
 */
int64_t col_indexof_str_str(const char *s, const char *needle);

/**
 * Removes leading and trailing whitespace from s. Whitespace means the six
 * ASCII bytes space, \t, \n, \v, \f and \r; every other byte, including
 * UTF-8 bytes, is text. Whitespace inside s is kept.
 *
 * @param s The string to strip.
 * @return A fresh block holding s without surrounding whitespace.
 */
char *col_strip_str(const char *s);

/**
 * Repeats s n times.
 *
 * @param s The string to repeat.
 * @param n How many copies to join. Non-positive yields the empty string.
 * @return A fresh block holding n copies of s.
 */
char *col_repeat_str_i64(const char *s, int64_t n);

/**
 * Replaces every occurrence of old_str in s with new_str. Occurrences are
 * found left to right and never overlap, and the inserted text is not
 * searched again: replacing "aa" with "b" in "aaa" gives "ba".
 *
 * An empty old_str matches nothing, so s comes back unchanged. Inserting text
 * between every byte would split multi-byte UTF-8 characters.
 *
 * @param s The string to search.
 * @param old_str The text to replace.
 * @param new_str The replacement text. May be empty, which deletes old_str.
 * @return A fresh block holding the result.
 */
char *col_replace_str_str_str(const char *s, const char *old_str, const char *new_str);

/**
 * Parses a base-10 integer, strictly. Apart from leading and trailing
 * whitespace (as for col_strip_str), s must hold one integer with an
 * optional sign and nothing else.
 *
 * Returns 0 for anything else, including "", "42abc", "1.5", "0x10" and
 * a value outside the int64_t range. A returned 0 therefore does not
 * tell "0" apart from invalid input.
 *
 * @param s The text to parse.
 * @return The parsed value, or 0.
 */
int64_t col_i64_str(const char *s);

/**
 * Parses a floating-point number, strictly. Apart from leading and
 * trailing whitespace (as for col_strip_str), s must hold one number in
 * strtod syntax and nothing else, e.g. "3.5", "-2.5e3" or ".5".
 *
 * Returns 0.0 for anything else, including "", "3.5x", and a value too
 * large for a double. Whether "inf", "nan" and hex floats are accepted
 * follows the platform's C library.
 *
 * @param s The text to parse.
 * @return The parsed value, or 0.0.
 */
double col_f64_str(const char *s);

/**
 * Converts an integer to its textual form, matching the formatting the
 * LLVM backend uses for println.
 *
 * @param x The value to convert.
 * @return A fresh block, e.g. "-42".
 */
char *col_string_i64(int64_t x);

/**
 * Converts a 128-bit integer to its textual form, in the same plain
 * decimal form as col_string_i64.
 *
 * No libc printf conversion specifier exists for a 128-bit integer, so
 * this function produces the digits itself rather than going through
 * snprintf.
 *
 * @param x The value to convert.
 * @return A fresh block, e.g. "-170141183460469231731687303715884105728".
 */
char *col_string_i128(__int128 x);

/**
 * Converts a float to its textual form, matching the formatting the LLVM
 * backend uses for println: printf's "%f", so six decimals.
 *
 * @param x The value to convert.
 * @return A fresh block, e.g. "3.140000".
 */
char *col_string_f64(double x);

/**
 * Converts a boolean to its textual form.
 *
 * Returns "true" or "false". Note that jcc's println currently prints
 * booleans as 1/0 (Bool.getFormat() returns "%d"); println is intended to
 * change to "true"/"false" to match this function. Until it does,
 * println(b) and println(string(b)) disagree — that is known, not a bug.
 *
 * @param x The value to convert.
 * @return A fresh block holding "true" or "false".
 */
char *col_string_bool(bool x);

/**
 * Reads one line from stdin, without the trailing newline.
 *
 * Handles arbitrarily long lines by growing its buffer, and returns a
 * final line that has no trailing newline intact.
 *
 * @return A fresh block holding the line, or the empty string at end of
 *         input.
 */
char *col_readln(void);

/**
 * Reports whether stdin is at end of input, WITHOUT consuming a
 * character. This is what lets a read loop terminate:
 *
 *     while not eof() do call println(readln()) end
 *
 * On an interactive terminal this blocks until a character is available,
 * which is inherent to peeking at stdin.
 *
 * @return true if stdin is at end of input, false otherwise.
 */
bool col_eof(void);

#ifdef __cplusplus
}
#endif

#endif /* JCCCOL_STRINGS_H */
