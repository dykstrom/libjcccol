/**
 * strings.c - Implementation of COL Standard Library string functions
 *
 * See jcccol/strings.h for the byte model, the never-null invariant, and
 * the fresh-block contract that shapes every function here.
 */

#include "jcccol/strings.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Allocation ----
 *
 * Every allocation in this file goes through col_alloc. A NULL from
 * malloc would violate the never-null invariant the rest of the runtime
 * relies on: jcc_gc_register returns NULL unchanged, so the NULL would be
 * stored into a root slot and crash later in unrelated code. Failing here
 * names the actual problem. */
static char *col_alloc(size_t size) {
    char *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "libjcccol: out of memory\n");
        exit(1);
    }
    return p;
}

/* Growing counterpart to col_alloc, with the same never-returns-NULL
 * contract. The old block is released on failure even though exit(1)
 * follows, so the two paths out of this function look the same to a
 * leak checker. */
static char *col_realloc(char *p, size_t size) {
    char *grown = realloc(p, size);
    if (grown == NULL) {
        free(p);
        fprintf(stderr, "libjcccol: out of memory\n");
        exit(1);
    }
    return grown;
}

/* A fresh, independently owned empty string. Never a shared constant —
 * the collector frees whatever it is handed. */
static char *col_empty(void) {
    char *result = col_alloc(1);
    result[0] = '\0';
    return result;
}

/* A fresh copy of the first `len` bytes of `s`, NUL-terminated. */
static char *col_copy(const char *s, size_t len) {
    char *result = col_alloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

/* The six ASCII whitespace bytes. Not isspace(), whose answer depends on
 * the locale and is undefined for negative char values. */
static bool col_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

/* True if only whitespace remains from p to the end of the string. */
static bool col_only_space(const char *p) {
    while (col_is_space(*p)) {
        p++;
    }
    return *p == '\0';
}

/* ---- Construction and slicing ---- */

char *col_concat_str_str(const char *s, const char *t) {
    size_t s_len = strlen(s);
    size_t t_len = strlen(t);
    char *result = col_alloc(s_len + t_len + 1);

    memcpy(result, s, s_len);
    memcpy(result + s_len, t, t_len);
    result[s_len + t_len] = '\0';

    return result;
}

char *col_substr_str_i64_i64(const char *s, int64_t start, int64_t length) {
    size_t s_len;
    size_t available;

    /* A negative start is not clamped to 0 on purpose: col_indexof_str_str
     * reports "absent" as -1, and feeding that in here should read as
     * "nothing found, nothing returned" rather than the first bytes of s. */
    if (start < 0 || length <= 0) {
        return col_empty();
    }

    s_len = strlen(s);
    if ((uint64_t) start >= s_len) {
        return col_empty();
    }

    /* Truncate a length that runs past the end. Both operands are known
     * to fit in size_t here: start < s_len, and length > 0. */
    available = s_len - (size_t) start;
    if ((uint64_t) length < available) {
        available = (size_t) length;
    }

    return col_copy(s + start, available);
}

int64_t col_indexof_str_str(const char *s, const char *needle) {
    const char *hit = strstr(s, needle);
    return (hit == NULL) ? -1 : (int64_t) (hit - s);
}

char *col_strip_str(const char *s) {
    const char *start = s;
    const char *end = s + strlen(s);

    while (start < end && col_is_space(*start)) {
        start++;
    }
    while (end > start && col_is_space(end[-1])) {
        end--;
    }
    return col_copy(start, (size_t) (end - start));
}

char *col_repeat_str_i64(const char *s, int64_t n) {
    size_t s_len = strlen(s);
    size_t total;
    char *result;
    int64_t i;

    if (n <= 0 || s_len == 0) {
        return col_empty();
    }

    /* Check before multiplying: an overflowed size would allocate a small
     * block and the copies below would run past it. Also covers an n
     * that does not fit in size_t at all. */
    if ((uint64_t) n > (SIZE_MAX - 1) / s_len) {
        fprintf(stderr, "libjcccol: out of memory\n");
        exit(1);
    }

    total = s_len * (size_t) n;
    result = col_alloc(total + 1);
    for (i = 0; i < n; i++) {
        memcpy(result + (size_t) i * s_len, s, s_len);
    }
    result[total] = '\0';
    return result;
}

char *col_replace_str_str_str(const char *s, const char *old_str, const char *new_str) {
    size_t old_len = strlen(old_str);
    size_t new_len = strlen(new_str);
    size_t count = 0;
    size_t total;
    const char *p;
    const char *hit;
    char *result;
    char *out;

    if (old_len == 0) {
        return col_copy(s, strlen(s));
    }

    /* Two passes over s: count the matches to size the block exactly,
     * then copy. Both passes skip past each match, so they agree on
     * which occurrences count. */
    for (p = s; (hit = strstr(p, old_str)) != NULL; p = hit + old_len) {
        count++;
    }

    /* The result is s with each match swapped, so its length is the old
     * length plus count * (new_len - old_len), written without a negative
     * intermediate. */
    total = strlen(s) - count * old_len + count * new_len;
    result = col_alloc(total + 1);

    out = result;
    for (p = s; (hit = strstr(p, old_str)) != NULL; p = hit + old_len) {
        memcpy(out, p, (size_t) (hit - p));
        out += hit - p;
        memcpy(out, new_str, new_len);
        out += new_len;
    }
    strcpy(out, p);
    return result;
}

/* ---- Conversions ----
 *
 * Sized with a snprintf(NULL, 0, ...) probe so the allocation is exact and
 * the format string lives in exactly one place per function. */

char *col_string_i64(int64_t x) {
    int needed = snprintf(NULL, 0, "%" PRId64, x);
    char *result = col_alloc((size_t) needed + 1);
    snprintf(result, (size_t) needed + 1, "%" PRId64, x);
    return result;
}

char *col_string_f64(double x) {
    /* "%f" matches F64.getFormat() in jcc, which is what println emits:
     * six decimals. */
    int needed = snprintf(NULL, 0, "%f", x);
    char *result = col_alloc((size_t) needed + 1);
    snprintf(result, (size_t) needed + 1, "%f", x);
    return result;
}

char *col_string_bool(bool x) {
    /* "true"/"false" rather than 1/0. jcc's println still prints 1/0
     * (Bool.getFormat() is "%d") and is intended to follow; see the note
     * in jcccol/strings.h. */
    return x ? col_copy("true", 4) : col_copy("false", 5);
}

/* The parsers are strict: apart from surrounding whitespace the whole
 * string must be consumed, otherwise the result is 0. strtoll and strtod
 * skip leading whitespace themselves; col_only_space checks the tail. */

int64_t col_i64_str(const char *s) {
    char *end;
    long long value;

    /* Base 10, not 0: base 0 would accept "0x10" as hex and "010" as
     * octal. With base 10, "0x10" stops after the "0" and the tail check
     * rejects it. */
    errno = 0;
    value = strtoll(s, &end, 10);
    if (end == s || errno == ERANGE || !col_only_space(end)) {
        return 0;
    }
    return (int64_t) value;
}

double col_f64_str(const char *s) {
    char *end;
    double value;

    errno = 0;
    value = strtod(s, &end);
    if (end == s || !col_only_space(end)) {
        return 0.0;
    }
    /* strtod also reports ERANGE on underflow, where the tiny value it
     * returns is still the right answer. Only overflow yields 0. */
    if (errno == ERANGE && (value == HUGE_VAL || value == -HUGE_VAL)) {
        return 0.0;
    }
    return value;
}

/* ---- Standard input ---- */

char *col_readln(void) {
    /* A hand-rolled growth loop rather than POSIX getline(), which is not
     * reliably available on MinGW. */
    size_t capacity = 128;
    size_t len = 0;
    char *buffer = col_alloc(capacity);
    int c;

    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        /* Keep one byte in reserve for the terminator. */
        if (len + 1 >= capacity) {
            capacity *= 2;
            buffer = col_realloc(buffer, capacity);
        }
        buffer[len++] = (char) c;
    }

    /* End of input with nothing read: the empty string, per the never-null
     * invariant. A final line without a trailing newline still returns its
     * bytes, because len > 0 here. */
    buffer[len] = '\0';
    return buffer;
}

bool col_eof(void) {
    /* Peek one byte and push it back. ungetc guarantees a single byte of
     * pushback, which is all this needs — do not extend it to peek
     * further without real buffering. */
    int c = fgetc(stdin);
    if (c == EOF) {
        return true;
    }
    ungetc(c, stdin);
    return false;
}
