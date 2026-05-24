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
- **Windows** (x86_64, via MSYS2/MinGW)

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

### Core Functions

#### `millis()`

Returns the number of milliseconds since the Unix epoch (January 1, 1970
00:00:00 UTC).

**Header:** `jcccol/core.h`
**Signature:** `int64_t millis(void)`
**Returns:** Milliseconds since epoch as a 64-bit signed integer.

**Example:**
```c
#include <jcccol.h>
#include <stdio.h>

int main(void) {
    int64_t now = millis();
    printf("Current time: %lld ms\n", (long long)now);
    return 0;
}
```

## Testing

The test suite uses a small custom framework. Each test binary returns 0
on success and prints detailed pass/fail information.

```bash
make test
```

## Integration with JCC

This library is designed to be linked with COL programs compiled by the
JCC compiler. A typical link line:

```bash
clang -o myprogram myprogram.o -L/path/to/libjcccol/build -ljcccol
```

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#relationship-to-jcc)
for the full integration picture.

## License

GPL-3.0. See [`LICENSE`](LICENSE).
