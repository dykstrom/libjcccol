# String runtime

`src/strings.c` and `include/jcccol/strings.h` implement COL's string
operations. JCC's LLVM backend emits calls to these symbols by name; the
per-function contracts are in the header's doc comments and are not
repeated here. This file records the four invariants that bind *any*
libjcccol function returning a string, including ones not written yet.

## The byte model

A COL string is an immutable, null-terminated, UTF-8, byte-transparent
`char *`. Functions operate on bytes and never inspect the encoding, so
non-ASCII text passes through byte-exact. `strlen` counts bytes, and
`col_substr_str_i64_i64` and `col_indexof_str_str` take and return
0-based byte offsets.

There is no codepoint API, and none is planned for v1 — no
`char_count`, no codepoint indexing, no normalization or locale-aware
collation. Splitting a multi-byte character with `col_substr_str_i64_i64`
is possible and is the caller's problem; the terminal does the decoding.

## Strings are never null

Every function returning a string returns a valid pointer to a
NUL-terminated block. Where there is nothing to return — a clamped
substring, `col_readln` at end of input — the result is the empty string.
COL has no `Optional` or nullable type, so there is no way to represent a
null string in the language and no caller prepared to check for one.

## Every returned string is a fresh block

The compiler wraps each result in `jcc_gc_register`, which takes
ownership of the block and eventually `free`s it. A returned pointer
therefore must never point into static storage, into a caller-supplied
buffer, or into a shared cache.

**This includes the empty string.** `col_empty()` in `src/strings.c`
allocates a new one-byte block every call. Returning a shared `""`
constant would be a double free the first time two empty results were
both collected. `tests/test_strings.c` guards this with
`empty_strings_are_distinct_blocks`, which asserts that separate calls
returning `""` return different pointers — the assertion a plausible
"just return a static empty string" optimization would break.

## Allocation failure exits

Every allocation goes through `col_alloc`, which prints
`libjcccol: out of memory` to stderr and calls `exit(1)` on failure.
Returning `NULL` instead would violate the never-null invariant:
`jcc_gc_register` passes NULL through unchanged, so it would be stored
into a root slot and crash later somewhere unrelated.

This differs deliberately from libjccbas, whose string functions
(`main/src/left.c` and siblings) do not check `malloc` at all.

## Reading stdin

`col_readln` grows its own buffer in an `fgetc` loop rather than using
POSIX `getline`, which is not reliably available on MinGW. A final line
with no trailing newline is returned intact.

`col_eof` peeks with `fgetc` and pushes back with `ungetc`. `ungetc`
guarantees exactly one byte of pushback, which is all this needs — do not
extend `col_eof` to look further ahead without real buffering. On an
interactive terminal it blocks until a character is available, which is
inherent to peeking at stdin.

Tests that exercise these two redirect stdin with `freopen` onto a temp
file, because `make test` runs each binary with stdin attached to the
terminal. Those tests run last in `tests/test_strings.c`, since `freopen`
on stdin cannot be undone.

## Known divergence: boolean formatting

`col_string_bool` returns `"true"` / `"false"`. JCC's `println` currently
prints booleans as `1` / `0` — `Bool.getFormat()` in
`jcc-base/.../types/Bool.java` returns `"%d"` — and is intended to change
to match. Until it does, `println(b)` and `println(string(b))` disagree
in a COL program. This is known, not a bug.
