# libjcccol - COL Standard Library

[![build](https://github.com/dykstrom/libjcccol/actions/workflows/build.yml/badge.svg)](https://github.com/dykstrom/libjcccol/actions/workflows/build.yml)
[![Latest Release](https://img.shields.io/github/v/release/dykstrom/libjcccol?display_name=release)](https://github.com/dykstrom/libjcccol/releases)
![Downloads](https://img.shields.io/github/downloads/dykstrom/libjcccol/total)
[![Open Issues](https://img.shields.io/github/issues/dykstrom/libjcccol)](https://github.com/dykstrom/libjcccol/issues)
![License](https://img.shields.io/github/license/dykstrom/libjcccol)
![Top Language](https://img.shields.io/github/languages/top/dykstrom/libjcccol)

The standard library for the COL programming language, designed for use
with the [JCC compiler](https://github.com/dykstrom/jcc).

`libjcccol` provides core runtime functionality for COL programs compiled
with JCC. It is implemented in portable C (C11) and built as a static
library (`libjcccol.a`) that can be linked with COL programs.

For repo layout, build-system design, release process, and JCC
integration details, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Platform Support

- **macOS** (arm64, x86_64)
- **Linux** (x86_64, arm64)
- **Windows** (x86_64 via MSYS2/MINGW64, arm64 via MSYS2/CLANGARM64)

## Building

### Prerequisites

- LLVM toolchain (Clang)
- Make
- `ar` and `ranlib` (standard archive tools)

### Commands

```bash
# Build the library (produces build/libjcccol.a)
make

# Build and run tests
make test

# Clean build artifacts
make clean

# Show all available targets
make help
```

## Library Functions

Every symbol `libjcccol` exports is prefixed `col_`, and functions that
COL can overload carry their parameter types in the name (for example
`col_indexof_str_str`). The full scheme is in
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#symbol-naming). The bundled
garbage collector is the one exception — it keeps its upstream
`jcc_gc_*` names.

### Core Functions

#### `col_millis()`

Returns the number of milliseconds since the Unix epoch (January 1, 1970
00:00:00 UTC).

**Header:** `jcccol/core.h`
**Signature:** `int64_t col_millis(void)`
**Returns:** Milliseconds since epoch as a 64-bit signed integer.

**Example:**
```c
#include <jcccol.h>
#include <stdio.h>

int main(void) {
    int64_t now = col_millis();
    printf("Current time: %lld ms\n", (long long)now);
    return 0;
}
```

### String Functions

A COL string is an immutable, null-terminated, UTF-8, byte-transparent
`char *`. All offsets and lengths are **0-based byte counts** — these
functions never inspect the encoding, so non-ASCII text passes through
unchanged. Every function returning a string returns a **fresh
`malloc`'d block** the caller owns, and never returns `NULL`; the empty
string is used where there is nothing to return.

String length and equality are deliberately absent — JCC emits calls to
libc's `strlen` and `strcmp` directly.

**Header:** `jcccol/strings.h`

| Function | Returns |
| --- | --- |
| `char *col_concat_str_str(const char *s, const char *t)` | `s` followed by `t` |
| `char *col_substr_str_i64_i64(const char *s, int64_t start, int64_t length)` | `length` bytes of `s` from byte offset `start` |
| `int64_t col_indexof_str_str(const char *s, const char *needle)` | Byte offset of the first occurrence, or `-1` |
| `char *col_string_i64(int64_t x)` | `x` as text, e.g. `"-42"` |
| `char *col_string_f64(double x)` | `x` as text with six decimals, e.g. `"3.140000"` |
| `char *col_string_bool(bool x)` | `"true"` or `"false"` |
| `char *col_readln(void)` | One line from stdin, without the trailing newline |
| `bool col_eof(void)` | Whether stdin is at end of input |

`col_substr_str_i64_i64` clamps rather than failing: a negative `start`,
a `start` at or past the end, or a non-positive `length` all yield the
empty string, and a `length` running past the end is truncated. The
negative-`start` rule makes the natural composition read correctly —
when `col_indexof_str_str` finds nothing it returns `-1`, and feeding
that into `col_substr_str_i64_i64` gives `""`.

`col_eof` peeks one byte and pushes it back, so it never consumes input.
That is what lets a read loop terminate:

```c
#include <jcccol.h>
#include <stdio.h>

int main(void) {
    while (!col_eof()) {
        char *line = col_readln();
        printf("%s\n", line);
        free(line);   /* in a COL program the collector owns this */
    }
    return 0;
}
```

Note: `col_string_bool` returns `"true"`/`"false"`, while JCC's `println`
currently prints booleans as `1`/`0`. `println` is intended to change to
match; until it does, the two disagree.

### Garbage Collector

`libjcccol` bundles the JCC runtime garbage collector — a precise
mark-sweep collector with compiler-managed roots — as `jcc_gc_*`. COL
programs do not call it directly; JCC emits the `jcc_gc_init`,
root-registration and frame calls itself when compiling a program that
uses heap types.

The collector is vendored verbatim from
[`libjccbas`](https://github.com/dykstrom/libjccbas), which remains the
canonical copy. The API contract is documented in
[`include/jcccol/jcc_gc.h`](include/jcccol/jcc_gc.h); provenance and the
re-vendoring procedure are in
[`docs/system/vendored-gc.md`](docs/system/vendored-gc.md).

## Testing

The test suite uses a small custom framework. Each test binary returns 0
on success and prints detailed pass/fail information.

```bash
make test
```

## Integration with JCC

The JCC compiler — a Maven-built Java/Kotlin project — consumes
`libjcccol` automatically. At JCC's build time, Maven detects the OS and
architecture, downloads the matching archive from this repo's
[Releases](https://github.com/dykstrom/libjcccol/releases) page
(`libjcccol-${version}-${classifier}.${type}`), extracts `libjcccol.a`,
and bundles it under JCC's `bin/`. JCC emits LLVM IR for COL programs
that references libjcccol symbols by name; the linker resolves them from
`libjcccol.a` at COL link time.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#relationship-to-jcc)
for the full integration picture, including the classifier/archive-type
table and what JCC actually consumes from the archive.

For C consumers using the headers directly, a typical link line:

```bash
clang -o myprogram myprogram.c -I/path/to/libjcccol/include \
      -L/path/to/libjcccol/build -ljcccol
```

## License

GPL-3.0. See [`LICENSE`](LICENSE).
