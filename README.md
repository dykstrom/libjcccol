# libjcccol - COL Standard Library

The standard library for the COL programming language, designed for use with the JCC compiler.

## Overview

`libjcccol` provides core runtime functionality for COL programs compiled with JCC. It is implemented in portable C and built as a static library (`libjcccol.a`) that can be linked with COL programs.

## Platform Support

- **macOS**
- **Linux**
- **Windows**

## Building

### Prerequisites

- LLVM toolchain (Clang)
- Make
- AR and ranlib (standard archive tools)

### Build Commands

```bash
# Build the library
make

# Build and run tests
make test

# Clean build artifacts
make clean

# Show all available targets
make help
```

The build produces `build/libjcccol.a` which can be linked with COL programs.

### Platform-Specific Notes

- **macOS**: Uses Clang
- **Linux**: Uses Clang
- **Windows**: Uses Clang; produces `.exe` test binaries

## Project Structure

```
libjcccol/
├── include/        # Public header files
│   ├── jcccol.h    # Main library header
│   └── jcccol/
│       └── core.h  # Core functionality
├── src/            # Implementation files
│   └── core.c      # Core functions
├── tests/          # Test files
│   └── test_core.c # Core function tests
├── build/          # Build artifacts (generated)
│   └── libjcccol.a # Output library
├── obj/            # Object files (generated)
├── Makefile        # Build configuration
└── README.md       # This file
```

## Library Functions

### Core Functions

#### `millis()`

Returns the number of milliseconds since the Unix epoch (January 1, 1970 00:00:00 UTC).

**Header:** `jcccol/core.h`
**Signature:** `int64_t millis(void)`
**Returns:** Milliseconds since epoch as a 64-bit signed integer

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

The test suite uses a simple custom test framework and provides comprehensive coverage of library functions.

```bash
# Run all tests
make test
```

Each test executable:
- Returns 0 on success, non-zero on failure
- Prints detailed pass/fail information
- Tests edge cases and cross-platform behavior

## Integration with JCC

This library is designed to be linked with COL programs compiled by the JCC compiler.

To link against this library:
```bash
clang -o myprogram myprogram.o -L/path/to/libjcccol/build -ljcccol
```

## Development

### Adding New Functions

1. Add function declaration to appropriate header in `include/jcccol/`
2. Implement function in corresponding source file in `src/`
3. Create tests in `tests/`
4. Run `make test` to verify

### Code Style

- C11 standard
- Snake_case for public functions (no prefix required)
- Comprehensive documentation in header files
- Cross-platform implementations using conditional compilation

### Compiler Flags

- `-Wall -Wextra -Werror`: All warnings enabled and treated as errors
- `-std=c11`: C11 standard compliance
- `-O2`: Optimization level 2

## License

[License information to be added]

## Contributing

[Contribution guidelines to be added]
