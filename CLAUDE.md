# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`libjcccol` is the standard library for the COL programming language, designed for use with the JCC compiler (located at `~/Workspace/jcc`). It follows similar patterns to the BASIC standard library at `~/Workspace/libjccbas`.

The library is:
- Written in portable C (C11 standard)
- Built using the LLVM toolchain (Clang on all platforms)
- Compiled to a static library (`libjcccol.a`)
- Cross-platform (macOS, Linux, Windows)
- Built with Make
- Licensed under GPL-3.0 (matches JCC)
- Primary development platform: **macOS / arm64**. Linux is a first-class
  target; Windows is supported but some convenience tooling (release script)
  is bash-only by design.

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

**Toolchain overrides:** `CC`, `AR`, and `RANLIB` are overridable from the
environment or command line (`make CC=gcc`). The Makefile uses
`ifeq ($(origin CC),default)` rather than `?=` for `CC` and `AR` because
make pre-defines them as built-ins (`cc`, `ar`); a plain `?=` would silently
yield gcc on Linux instead of clang.

### Testing Individual Functions
```bash
# Build a specific test
make build/test_core

# Run it directly
./build/test_core
```

### Development Workflow
1. Make changes to source files in `src/` or headers in `include/jcccol/`
2. Run `make clean && make` to rebuild
3. Run `make test` to verify changes
4. All compiler warnings are treated as errors (`-Werror`)

## Architecture

### Directory Structure
```
include/             # Public headers (what JCC will include)
├── jcccol.h         # Main umbrella header
└── jcccol/          # Modular headers
    └── core.h       # Core time/utility functions

src/                 # Implementation files (platform-specific code here)
└── core.c           # Core implementations with Windows/POSIX variants

tests/               # Test suite using custom framework
└── test_core.c      # Tests for core functions

scripts/             # Helper scripts (release automation, etc.)
└── release.sh       # Release script — bash, macOS/Linux only

.github/workflows/   # GitHub Actions
├── build.yml        # Matrix build on push/PR (5 platforms)
└── release.yml      # Tag-triggered release; uploads to GitHub Releases

docs/                # Reviews, design notes
VERSION              # Single source of truth for the library version
LICENSE              # GPL-3.0

build/               # Generated: output artifacts
└── libjcccol.a      # The library artifact used by JCC

obj/                 # Generated: object files
dist/                # Generated (CI): staged release archives
```

### Code Organization

**Public API**: Public functions do NOT use a prefix and are declared in headers under `include/jcccol/`. The main `jcccol.h` includes all subheaders.

**Platform Handling**: Platform-specific code uses conditional compilation with `#ifdef _WIN32` for Windows and `#else` for POSIX systems (macOS/Linux).

**Naming Conventions**:
- Public functions: `functionname()` (snake_case, no prefix)
- Header guards: `JCCCOL_MODULE_H` pattern
- Internal/static functions: can use any reasonable naming

### Build System

The Makefile automatically:
- Uses `clang` on all platforms (macOS, Linux, Windows)
- Creates necessary directories
- Handles cross-platform test executable naming (.exe on Windows)
- Generates dependency files for incremental builds

Compiler flags: `-Wall -Wextra -Werror -std=c11 -O2`

### Release Process

Releases are cut by tagging a commit on `master` or `main`. The
`.github/workflows/release.yml` workflow triggers on `v*` tags, builds the
5-platform matrix (Windows x86_64, macOS arm64, macOS x86_64, Linux x86_64,
Linux arm64), runs tests, packages `lib/`, `include/`, `README.md`, and
`LICENSE` per platform, and uploads to a GitHub Release. No `gh` CLI is
required locally.

**Single source of truth for the version:** the `VERSION` file at the repo
root. The Makefile reads it via `$(shell cat VERSION)`. Bump it via the
release script — do not edit it by hand outside of a release.

**Standard flow (macOS / Linux):**

```bash
# 1. From a clean tree on master/main:
make release NEW_VERSION=0.2.0
# The script:
#   - validates clean tree, branch, and that v0.2.0 doesn't already exist
#     locally or on origin
#   - runs `make clean && make test`
#   - writes 0.2.0 to VERSION
#   - creates a commit "Release v0.2.0" and an annotated tag v0.2.0
#   - prints the push command; does NOT push automatically

# 2. Review:
git show HEAD
git tag -n1 v0.2.0

# 3. Publish (this is what triggers the CI release):
git push --follow-tags origin master

# 4. (If something is wrong before pushing, abort with:)
git tag -d v0.2.0 && git reset --hard HEAD~1
```

**Windows:** the release script is intentionally bash-only. To cut a release
from Windows, run the equivalent steps manually:

```bash
echo X.Y.Z > VERSION
git commit -am "Release vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push --follow-tags
```

### Open Design Decisions

Documented here so a future Claude session doesn't have to rediscover them.
These are flagged by the architecture review at
`docs/REVIEW-claude-opus-4-7-2026-05-22.md`.

- **Symbol prefix policy.** Public functions currently have no `jcccol_`
  prefix (per the convention in this file). This works for `millis()` but
  will collide with libc / common user code once functions like `sleep`,
  `print`, or `read_line` are added. Decide before the second public
  function lands. Options: (a) keep unprefixed and trust link-time
  isolation, (b) prefix everything `col_`, (c) keep convenient names in
  headers and use static-inline wrappers around prefixed link symbols.
- **No `version()` API yet.** The `VERSION` file exists and the Makefile
  reads it, but it is not yet passed as `-DJCCCOL_VERSION_STRING=...` and
  no C-level accessor exists. Add both together when the first consumer
  needs the version at runtime.
- **No `V=1` verbose toggle.** The review (§4.2 #5) proposed a kernel-style
  `Q := @` toggle so commands are silent by default and `V=1` re-enables
  echoing. We skipped it: the premise was wrong — recipes already echo the
  full `clang` invocation today (only the status `@echo` lines are
  suppressed). Revisit if recipe output becomes genuinely noisy.

### Test Framework

Uses a simple custom test framework defined in test files:
- `TEST(name)` macro defines a test
- `RUN_TEST(name)` executes and reports results
- `ASSERT(condition)` and `ASSERT_MSG(condition, msg)` for assertions
- Tests return 0 on success, 1 on failure

### Development Guidelines

- **ALWAYS make a plan before executing changes.** State what you're going
  to change and why before any file-modifying tool call. For multi-step
  work, write the plan out (a short message, or `TaskCreate` for longer
  task lists) and get the user's confirmation before proceeding. This
  applies to every kind of change — new functionality, bug fixes,
  refactors, build-system tweaks, docs.

### Adding New Functionality

When adding new functions to the library:

1. **Add to appropriate header** (or create new module under `include/jcccol/`)
   - Use snake_case naming (no prefix)
   - Include full documentation comments
   - Consider platform differences upfront

2. **Implement in src/** (or create new .c file)
   - Use `#ifdef _WIN32` for Windows-specific code
   - Use POSIX APIs for macOS/Linux
   - Keep platform abstractions clean

3. **Add to jcccol.h** if creating new module
   - Include the new subheader

4. **Create tests in tests/**
   - Name test file `test_modulename.c`
   - Use the test framework macros
   - Test cross-platform behavior
   - Test edge cases

5. **Verify build**
   ```bash
   make clean && make test
   ```

### Relationship to JCC

This library will be linked with COL programs compiled by the JCC compiler. JCC will:
- Include headers from `include/`
- Link against `build/libjcccol.a`
- Use library functions as runtime support for COL language features
