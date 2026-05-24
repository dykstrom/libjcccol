# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working
with code in this repository.

## Project Overview

`libjcccol` is the standard library for the COL programming language,
designed for use with the JCC compiler. Architecture details (layout,
build system, release process, JCC integration, open design decisions)
live in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md). Read that first
when orienting on this repo.

## Development Commands

### Build Commands
```bash
# Build the library (creates build/libjcccol.a)
make

# Build and run all tests
make test

# Clean all build artifacts
make clean

# Show help with all available targets
make help

# Print current library version (read from the VERSION file)
make version

# Cut a release: bumps VERSION, commits, tags. macOS/Linux only.
# Does NOT push; review with `git show` and push manually to trigger CI.
make release NEW_VERSION=0.2.0

# Stage and archive a release bundle (used by CI; layout defined in Makefile).
make dist PLATFORM=macos-arm64 ARCHIVE=tar.gz
```

### Testing Individual Functions
```bash
# Build a specific test
make build/test_core

# Run it directly
./build/test_core
```

### Development Workflow
1. Make changes to source files in `src/` or headers in `include/jcccol/`.
2. Run `make clean && make` to rebuild.
3. Run `make test` to verify changes.
4. All compiler warnings are treated as errors (`-Werror`).

## Development Guidelines

- **ALWAYS make a plan before executing changes.** State what you're going
  to change and why before any file-modifying tool call. For multi-step
  work, write the plan out (a short message, or `TaskCreate` for longer
  task lists) and get the user's confirmation before proceeding. This
  applies to every kind of change — new functionality, bug fixes,
  refactors, build-system tweaks, docs.

## Adding New Functionality

For where files live and the platform-abstraction conventions, see
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md). The checklist for adding a
new public function:

1. **Add to the appropriate header** under `include/jcccol/` (or create a
   new module header there). Use snake_case, no prefix. Include doc
   comments. Consider platform differences upfront.
2. **Implement in `src/`** — use `#ifdef _WIN32` for Windows-specific
   code, POSIX APIs for macOS/Linux.
3. **Add the new subheader to `include/jcccol.h`** if you created a new
   module.
4. **Create tests in `tests/`** — name the file `test_modulename.c`, use
   the test framework macros, cover cross-platform behavior and edge
   cases.
5. **Verify the build:** `make clean && make test`.
