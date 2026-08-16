/**
 * strings.c - Implementation of COL Standard Library string functions
 *
 * See jcccol/strings.h for the byte model, the never-null invariant, and
 * the fresh-block contract that shapes every function here.
 */

#include "jcccol/strings.h"

#include <inttypes.h>
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
