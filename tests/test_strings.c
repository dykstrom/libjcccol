/**
 * test_strings.c - Tests for COL Standard Library string functions
 *
 * Two groups, in order. The pure tests come first; the stdin tests come
 * last because each one calls freopen() on stdin, which cannot be undone.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jcccol.h"
#include "test_framework.h"

/* Every string this library returns is a fresh malloc'd block the caller
 * owns, so tests free what they allocate. ASSERT returns early on
 * failure, which leaks — acceptable in a test binary that exits anyway,
 * and it keeps the assertions readable. */
#define ASSERT_STR_EQ(actual, expected) do { \
    char *a_ = (actual); \
    const char *e_ = (expected); \
    if (strcmp(a_, e_) != 0) { \
        printf("\n    Assertion failed: expected \"%s\", got \"%s\"\n", e_, a_); \
        free(a_); \
        return 1; \
    } \
    free(a_); \
} while (0)

/* ---- Concatenation ---- */

TEST(concat_basic) {
    ASSERT_STR_EQ(col_concat_str_str("hello", " world"), "hello world");
    return 0;
}

TEST(concat_empty) {
    ASSERT_STR_EQ(col_concat_str_str("", "x"), "x");
    ASSERT_STR_EQ(col_concat_str_str("x", ""), "x");
    ASSERT_STR_EQ(col_concat_str_str("", ""), "");
    return 0;
}

/* ---- Substring ---- */

TEST(substr_basic) {
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello world", 0, 5), "hello");
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello world", 6, 5), "world");
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 0, 5), "hello");
    return 0;
}

/* Every row of the clamping table in jcccol/strings.h. */
TEST(substr_clamping) {
    /* negative start */
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", -1, 3), "");
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", -100, 3), "");
    /* start at or past the end */
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 5, 3), "");
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 99, 3), "");
    /* non-positive length */
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 1, 0), "");
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 1, -5), "");
    /* length running past the end is truncated */
    ASSERT_STR_EQ(col_substr_str_i64_i64("hello", 3, 99), "lo");
    /* empty source */
    ASSERT_STR_EQ(col_substr_str_i64_i64("", 0, 5), "");
    return 0;
}

/* The composition the negative-start rule exists for: a needle that is
 * absent yields -1, which must read as "nothing found, nothing returned"
 * rather than the first bytes of the string. */
TEST(substr_indexof_composition) {
    const char *s = "hello world";
    int64_t absent = col_indexof_str_str(s, "xyz");
    int64_t present = col_indexof_str_str(s, "world");

    ASSERT(absent == -1);
    ASSERT_STR_EQ(col_substr_str_i64_i64(s, absent, 5), "");
    ASSERT_STR_EQ(col_substr_str_i64_i64(s, present, 5), "world");
    return 0;
}

/* ---- indexof ---- */

TEST(indexof_basic) {
    ASSERT(col_indexof_str_str("hello world", "world") == 6);
    ASSERT(col_indexof_str_str("hello world", "hello") == 0);
    ASSERT(col_indexof_str_str("hello world", "o") == 4);
    ASSERT(col_indexof_str_str("aaa", "aa") == 0);
    return 0;
}

TEST(indexof_edge_cases) {
    ASSERT_MSG(col_indexof_str_str("hello", "xyz") == -1, "absent needle is -1");
    ASSERT_MSG(col_indexof_str_str("hello", "") == 0, "empty needle is 0");
    ASSERT_MSG(col_indexof_str_str("", "") == 0, "empty in empty is 0");
    ASSERT_MSG(col_indexof_str_str("", "x") == -1, "empty haystack is -1");
    ASSERT_MSG(col_indexof_str_str("hi", "hello") == -1, "needle longer is -1");
    return 0;
}

/* ---- Conversions ---- */

TEST(string_i64) {
    ASSERT_STR_EQ(col_string_i64(0), "0");
    ASSERT_STR_EQ(col_string_i64(42), "42");
    ASSERT_STR_EQ(col_string_i64(-42), "-42");
    ASSERT_STR_EQ(col_string_i64(INT64_MAX), "9223372036854775807");
    ASSERT_STR_EQ(col_string_i64(INT64_MIN), "-9223372036854775808");
    return 0;
}

/* "%f" — six decimals, matching F64.getFormat() in jcc. */
TEST(string_f64) {
    ASSERT_STR_EQ(col_string_f64(3.14), "3.140000");
    ASSERT_STR_EQ(col_string_f64(0.0), "0.000000");
    ASSERT_STR_EQ(col_string_f64(-2.5), "-2.500000");
    ASSERT_STR_EQ(col_string_f64(1.0), "1.000000");
    return 0;
}

/* "true"/"false", not 1/0. jcc's println still prints 1/0 and is intended
 * to follow — see the note in jcccol/strings.h. */
TEST(string_bool) {
    ASSERT_STR_EQ(col_string_bool(true), "true");
    ASSERT_STR_EQ(col_string_bool(false), "false");
    return 0;
}

/* ---- The fresh-block contract ---- */

/* Two calls that both produce "" must return DIFFERENT pointers. The
 * collector frees whatever it is handed, so a shared static empty string
 * would be a double free. This is the test a naive optimization breaks. */
TEST(empty_strings_are_distinct_blocks) {
    char *a = col_substr_str_i64_i64("hello", -1, 3);
    char *b = col_substr_str_i64_i64("hello", 99, 3);
    char *c = col_concat_str_str("", "");

    ASSERT(strcmp(a, "") == 0);
    ASSERT(strcmp(b, "") == 0);
    ASSERT(strcmp(c, "") == 0);
    ASSERT_MSG(a != b, "each empty result must be its own block");
    ASSERT_MSG(b != c, "each empty result must be its own block");
    ASSERT_MSG(a != c, "each empty result must be its own block");

    free(a);
    free(b);
    free(c);
    return 0;
}

/* A returned block must not alias its argument. */
TEST(results_do_not_alias_arguments) {
    const char *source = "hello";
    char *copy = col_substr_str_i64_i64(source, 0, 5);

    ASSERT(strcmp(copy, source) == 0);
    ASSERT_MSG((const char *) copy != source, "must be a fresh block");

    free(copy);
    return 0;
}

/* ---- UTF-8 byte transparency ---- */

/* "smörgås" in UTF-8: ö and å are two bytes each, so strlen is 9, not 7.
 * Written as explicit escapes so the test does not depend on this file's
 * own encoding. */
TEST(utf8_is_byte_transparent) {
    const char *s = "sm\xc3\xb6rg\xc3\xa5s";
    char *joined;

    ASSERT_MSG(strlen(s) == 9, "7 characters, 9 bytes");

    /* Concatenation preserves the bytes exactly. */
    joined = col_concat_str_str(s, "!");
    ASSERT(strcmp(joined, "sm\xc3\xb6rg\xc3\xa5s!") == 0);
    ASSERT(strlen(joined) == 10);
    free(joined);

    /* Offsets are bytes: "sm" is 0..1, and the ö occupies bytes 2..3. */
    ASSERT_STR_EQ(col_substr_str_i64_i64(s, 0, 2), "sm");
    ASSERT_STR_EQ(col_substr_str_i64_i64(s, 2, 2), "\xc3\xb6");

    /* indexof reports a byte offset too. */
    ASSERT(col_indexof_str_str(s, "rg") == 4);
    return 0;
}

/* ---- stdin ----
 *
 * These run last: each replaces stdin via freopen, which cannot be undone.
 * Binary mode keeps the bytes exact on Windows. */

static const char *const STDIN_PATH = "test_strings_stdin.tmp";

static int set_stdin(const char *content) {
    FILE *f = fopen(STDIN_PATH, "wb");
    if (f == NULL) {
        return 1;
    }
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return freopen(STDIN_PATH, "rb", stdin) == NULL;
}

TEST(readln_basic) {
    ASSERT(set_stdin("hello\nworld\n") == 0);
    ASSERT_STR_EQ(col_readln(), "hello");
    ASSERT_STR_EQ(col_readln(), "world");
    return 0;
}

/* An empty line is a line, not end of input. */
TEST(readln_empty_line) {
    ASSERT(set_stdin("\nx\n") == 0);
    ASSERT_STR_EQ(col_readln(), "");
    ASSERT_STR_EQ(col_readln(), "x");
    return 0;
}

/* Longer than the 128-byte initial buffer, so the growth path runs. */
TEST(readln_long_line) {
    char expected[1000];
    char input[1002];
    size_t i;

    for (i = 0; i < sizeof(expected) - 1; i++) {
        expected[i] = (char) ('a' + (i % 26));
    }
    expected[sizeof(expected) - 1] = '\0';

    snprintf(input, sizeof(input), "%s\n", expected);
    ASSERT(set_stdin(input) == 0);

    ASSERT_STR_EQ(col_readln(), expected);
    return 0;
}

/* A final line with no trailing newline comes back intact. */
TEST(readln_no_trailing_newline) {
    ASSERT(set_stdin("abc") == 0);
    ASSERT_STR_EQ(col_readln(), "abc");
    return 0;
}

TEST(readln_empty_stdin) {
    ASSERT(set_stdin("") == 0);
    ASSERT_STR_EQ(col_readln(), "");
    return 0;
}

TEST(eof_empty_stdin) {
    ASSERT(set_stdin("") == 0);
    ASSERT_MSG(col_eof(), "empty stdin is at end of input");
    return 0;
}

/* The property the read loop depends on: eof() peeks without consuming,
 * so the following readln() still sees the whole line. */
TEST(eof_does_not_consume) {
    ASSERT(set_stdin("line one\n") == 0);
    ASSERT_MSG(!col_eof(), "a character is waiting");
    ASSERT_MSG(!col_eof(), "repeated peeks stay non-destructive");
    ASSERT_STR_EQ(col_readln(), "line one");
    ASSERT_MSG(col_eof(), "input is exhausted");
    return 0;
}

/* The epic's motivating loop: while not eof() do println(readln()) end */
TEST(eof_readln_loop_terminates) {
    const char *expected[] = { "alpha", "beta", "gamma" };
    int seen = 0;

    ASSERT(set_stdin("alpha\nbeta\ngamma\n") == 0);
    while (!col_eof()) {
        char *line = col_readln();
        ASSERT(seen < 3);
        ASSERT(strcmp(line, expected[seen]) == 0);
        free(line);
        seen++;
    }
    ASSERT_MSG(seen == 3, "loop should read exactly three lines");
    return 0;
}

/* A last line without a trailing newline must not be lost by the loop. */
TEST(eof_readln_loop_without_trailing_newline) {
    int seen = 0;

    ASSERT(set_stdin("one\ntwo") == 0);
    while (!col_eof()) {
        char *line = col_readln();
        free(line);
        seen++;
    }
    ASSERT_MSG(seen == 2, "the unterminated final line still counts");
    return 0;
}

int main(void) {
    int total = 0;
    int passed = 0;
    int failed = 0;

    printf("\nRunning string library tests...\n");
    printf("================================\n\n");

    RUN_TEST(concat_basic);
    RUN_TEST(concat_empty);
    RUN_TEST(substr_basic);
    RUN_TEST(substr_clamping);
    RUN_TEST(substr_indexof_composition);
    RUN_TEST(indexof_basic);
    RUN_TEST(indexof_edge_cases);
    RUN_TEST(string_i64);
    RUN_TEST(string_f64);
    RUN_TEST(string_bool);
    RUN_TEST(empty_strings_are_distinct_blocks);
    RUN_TEST(results_do_not_alias_arguments);
    RUN_TEST(utf8_is_byte_transparent);

    /* Must run last: each replaces stdin via freopen. */
    RUN_TEST(readln_basic);
    RUN_TEST(readln_empty_line);
    RUN_TEST(readln_long_line);
    RUN_TEST(readln_no_trailing_newline);
    RUN_TEST(readln_empty_stdin);
    RUN_TEST(eof_empty_stdin);
    RUN_TEST(eof_does_not_consume);
    RUN_TEST(eof_readln_loop_terminates);
    RUN_TEST(eof_readln_loop_without_trailing_newline);

    remove(STDIN_PATH);

    printf("\n================================\n");
    printf("Results: %d/%d tests passed", passed, total);
    if (failed > 0) {
        printf(", %d FAILED", failed);
    }
    printf("\n\n");

    return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
