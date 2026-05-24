# libjcccol Architecture

This document is the single source of truth for how `libjcccol` is laid
out, built, tested, and released, and how it integrates with the JCC
compiler. `CLAUDE.md` and `README.md` reference this file rather than
duplicating its content.

## Overview

`libjcccol` is the standard library for the COL programming language,
designed for use with the [JCC compiler](https://github.com/dykstrom/jcc).
It follows the same pattern as the BASIC standard library
[`libjccbas`](https://github.com/dykstrom/libjccbas).

The library is:

- Written in portable C (C11 standard).
- Built using the LLVM toolchain (Clang on all platforms).
- Compiled to a static library (`libjcccol.a`).
- Cross-platform (macOS, Linux, Windows).
- Built with Make.
- Licensed under GPL-3.0 (matches JCC).
- Primary development platform: **macOS / arm64**. Linux is a first-class
  target; Windows is supported but some convenience tooling (the release
  script) is bash-only by design.

## Directory Structure

```
include/             # Public headers (what JCC will include)
├── jcccol.h         # Umbrella header
└── jcccol/          # Modular headers
    └── core.h       # Core time/utility functions

src/                 # Implementation files (platform-specific code lives here)
└── core.c           # Core implementations with Windows/POSIX variants

tests/               # Test suite using a custom in-tree framework
├── test_framework.h
└── test_core.c

scripts/             # Helper scripts (release automation, etc.)
└── release.sh       # Release script — bash, macOS/Linux only

.github/workflows/   # GitHub Actions
├── build.yml        # Matrix build on push/PR (5 platforms)
└── release.yml      # Tag-triggered release; uploads to GitHub Releases

docs/                # Reviews, design notes, this document
VERSION              # Single source of truth for the library version
LICENSE              # GPL-3.0

build/               # Generated: output artifacts
└── libjcccol.a      # The library artifact used by JCC

obj/                 # Generated: object files (src and tests/)
dist/                # Generated (local / CI): staged release archives
```

## Code Organization

### Public API

Public functions do **not** use a prefix and are declared in headers under
`include/jcccol/`. The umbrella header `include/jcccol.h` includes all
subheaders so callers can `#include <jcccol.h>` without worrying about
modules.

### Platform Handling

Platform-specific code uses conditional compilation: `#ifdef _WIN32` for
Windows, `#else` for POSIX systems (macOS, Linux). Keep the platform
abstractions thin and inside the implementation file — the public headers
should be platform-agnostic.

The POSIX branch defines `_POSIX_C_SOURCE 200809L` before including system
headers so glibc exposes POSIX.1-2008 symbols (e.g. `clock_gettime`) under
`-std=c11`. macOS does not need this; MinGW tolerates it.

### Naming Conventions

- Public functions: `functionname()` (snake_case, no prefix).
- Header guards: `JCCCOL_MODULE_H` pattern.
- Internal/static functions: any reasonable naming.

## Build System

The Makefile drives everything. Highlights:

- Uses Clang on all platforms (`clang` is the default `CC`).
- Auto-discovers `.c` files in `src/` and `tests/`.
- Compiler flags: `-Wall -Wextra -Werror -std=c11 -O2 -MMD -MP`. Warnings
  are errors; `-MMD -MP` emits `.d` dependency files as a side-effect of
  compilation, so dependency tracking stays in lock-step with the build.
- Test objects live under `obj/tests/` to avoid colliding with `src/`
  objects of the same name.
- The test pattern rule splits compile (`obj/tests/%.o`) from link
  (`build/%`), so a test can grow to multiple translation units.
- Cross-platform test executable naming (`.exe` on Windows) is handled by
  the `EXE` suffix variable, which is detected from `uname -s` /
  `OS=Windows_NT`.

### Toolchain Overrides

`CC`, `AR`, and `RANLIB` are overridable from the environment or command
line (`make CC=gcc`). The Makefile uses `ifeq ($(origin CC),default)`
rather than `?=` for `CC` and `AR` because make pre-defines them as
built-ins (`cc`, `ar`); a plain `?=` would silently yield gcc on Linux
instead of clang.

### Dist Bundles

`make dist PLATFORM=<name> ARCHIVE=tar.gz|zip` stages a release bundle
under `dist/libjcccol-<PLATFORM>/` and produces an archive. The layout
(`lib/`, `include/`, `README.md`, `LICENSE`) is defined in one place in
the Makefile so CI workflows do not duplicate it.

The default `PLATFORM` normalizes `uname -s` (`darwin → macos`,
`mingw*`/`cygwin*`/`Windows_NT → windows`) so a local `make dist` produces
archive names that match the CI matrix.

## Test Framework

A small custom framework defined in `tests/test_framework.h`:

- `TEST(name)` defines a test.
- `RUN_TEST(name)` executes it and reports the result.
- `ASSERT(condition)` and `ASSERT_MSG(condition, msg)` for assertions.
- Each test binary returns 0 on success, non-zero on failure.

## Release Process

Releases are cut by tagging a commit on `master` or `main`.
`.github/workflows/release.yml` triggers on `v*` tags, builds the
5-platform matrix (Windows x86_64, macOS arm64, macOS x86_64, Linux
x86_64, Linux arm64), runs tests, calls `make dist`, and uploads the
archives to a GitHub Release. No `gh` CLI is required locally.

**Single source of truth for the version:** the `VERSION` file at the
repo root. The Makefile reads it via `$(shell cat VERSION)`. Bump it via
the release script — do not edit it by hand outside of a release.

### Standard Flow (macOS / Linux)

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

### Windows

The release script is intentionally bash-only. To cut a release from
Windows, run the equivalent git steps manually:

```bash
echo X.Y.Z > VERSION
git commit -am "Release vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push --follow-tags
```

## Relationship to JCC

This library is linked with COL programs compiled by the JCC compiler.
JCC will:

- Include headers from `include/`.
- Link against `build/libjcccol.a` (or the equivalent file from a release
  bundle).
- Use library functions as runtime support for COL language features.

A typical link line:

```bash
clang -o myprogram myprogram.o -L/path/to/libjcccol/build -ljcccol
```

## Open Design Decisions

Documented here so a future contributor doesn't have to rediscover them.
These are tracked against the architecture review at
`docs/REVIEW-claude-opus-4-7-2026-05-22.md`.

- **Symbol prefix policy.** Public functions currently have no `jcccol_`
  prefix. This works for `millis()` but will collide with libc / common
  user code once functions like `sleep`, `print`, or `read_line` are
  added. Decide before the second public function lands. Options: (a)
  keep unprefixed and trust link-time isolation, (b) prefix everything
  `col_`, (c) keep convenient names in headers and use static-inline
  wrappers around prefixed link symbols.
- **No `version()` API yet.** The `VERSION` file exists and the Makefile
  reads it, but it is not yet passed as `-DJCCCOL_VERSION_STRING=...` and
  no C-level accessor exists. Add both together when the first consumer
  needs the version at runtime.
- **No `V=1` verbose toggle.** The review (§4.2 #5) proposed a
  kernel-style `Q := @` toggle so commands are silent by default and
  `V=1` re-enables echoing. We skipped it: the premise was wrong —
  recipes already echo the full `clang` invocation today (only the
  status `@echo` lines are suppressed). Revisit if recipe output becomes
  genuinely noisy.
