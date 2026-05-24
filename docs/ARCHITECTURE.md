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

A small custom framework in [`tests/test_framework.h`](../tests/test_framework.h):
`TEST`/`RUN_TEST`/`ASSERT`/`ASSERT_MSG` macros plus a `test_sleep_ms`
helper. Test binaries return 0 on success.

## Release Process

Releases are cut by tagging a commit on `master` or `main`.
`.github/workflows/release.yml` triggers on `v*` tags, builds the
6-platform matrix (Windows x86_64, Windows arm64, macOS arm64, macOS
x86_64, Linux x86_64, Linux arm64), runs tests, calls `make dist`, and
uploads archives named `libjcccol-${version}-${classifier}.${type}` to a
GitHub Release. No `gh` CLI is required locally.

**Single source of truth for the version:** the `VERSION` file at the
repo root. The Makefile reads it via `$(shell cat VERSION)`. Bump it via
the release script — do not edit it by hand outside of a release.

### Standard Flow (macOS / Linux)

From a clean tree on `master`/`main`:

```bash
make release NEW_VERSION=0.2.0   # validates, tests, bumps VERSION, commits, tags — does NOT push
git show HEAD && git tag -n1 v0.2.0   # review
git push --follow-tags origin master  # publish (triggers CI release)
```

To abort before pushing: `git tag -d v0.2.0 && git reset --hard HEAD~1`.
See [`scripts/release.sh`](../scripts/release.sh) for the exact checks.

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

`libjcccol` is consumed by [JCC](https://github.com/dykstrom/jcc) — a
Maven-built Java/Kotlin project — via per-platform release archives
published to GitHub Releases on tags. The integration is the same one
JCC uses for [`libjccbas`](https://github.com/dykstrom/libjccbas) and is
wired in JCC's `jcc-compiler/pom.xml`. The current reference is on the
[`llvm-basic`](https://github.com/dykstrom/jcc/blob/llvm-basic/jcc-compiler/pom.xml)
branch.

### Integration mechanics

1. **Tag pushed to `master`/`main` here** triggers
   `.github/workflows/release.yml`, which builds the matrix and publishes
   a GitHub Release with one archive per platform.
2. **JCC's `jcc-compiler` module** pins a `libjcccol` version. At Maven
   build time, OS/arch detection activates a per-platform profile that
   sets `native.classifier` and `native.archive.type`.
3. **JCC's Maven build** first looks in the local Maven repo at
   `~/.m2/repository/se/dykstrom/jcc/libjcccol/${version}/libjcccol-${version}-${classifier}.${type}`.
   If absent, it downloads from
   `https://github.com/dykstrom/libjcccol/releases/download/v${version}/libjcccol-${version}-${classifier}.${type}`
   and installs the archive into the local Maven repo via
   `mvn install:install-file`.
4. **`maven-dependency-plugin:unpack`** extracts the archive into JCC's
   `target/temp-extract/`. An antrun step then flattens out the
   `libjcccol.*` files into `target/`.
5. **`maven-resources-plugin`** copies `libjcccol.a` (and any Windows
   `libjcccol.dll` variants, should they exist later) into JCC's `bin/`
   directory, so the library ships alongside the JCC distribution.

### Archive naming and platforms

JCC expects archives named `libjcccol-${version}-${classifier}.${type}`,
where `${version}` matches the git tag (without the leading `v`) and
`${classifier}` is one of:

| OS family | Architecture | Classifier        | Archive type |
| --- | --- | --- | --- |
| Windows | x86_64 | `windows-x86_64` | `zip` |
| Windows | arm64  | `windows-arm64`  | `zip` |
| macOS   | x86_64 | `macos-x86_64`   | `tar.gz` |
| macOS   | arm64  | `macos-arm64`    | `tar.gz` |
| Linux   | x86_64 | `linux-x86_64`   | `tar.gz` |
| Linux   | arm64  | `linux-arm64`    | `tar.gz` |

Both `make dist` and the release workflow produce filenames in this
format. `VERSION` is the single source of truth and is read by the
Makefile.

### What's in the archive (and what JCC actually uses)

Each archive contains:

```
libjcccol-<version>-<classifier>/
├── lib/libjcccol.a   ← the only file JCC consumes
├── include/jcccol.h
├── include/jcccol/*.h
├── README.md
└── LICENSE
```

JCC's antrun step uses `<copy flatten="true">` with `**/libjcccol.*`, so
the layout inside the archive is flexible — only the presence of
`libjcccol.a` (anywhere) matters. JCC does **not** consume the headers:
its COL compile path emits LLVM IR that calls libjcccol symbols by name,
and the linker resolves them from `libjcccol.a` at COL link time. The
headers are bundled for human/C consumers and for documenting the API.

Implications for compatibility:

- **Renaming or removing an exported symbol breaks JCC.** Treat
  exported-symbol changes as ABI changes.
- **Adding new exported symbols is safe** — JCC ignores anything it
  doesn't reference.
- **Header-only changes are invisible to JCC** (since JCC doesn't compile
  against them). They still matter for any human/C consumer.

### Linking against the static library directly

For C consumers who do use the headers (test programs, hand-written C
that calls libjcccol), the in-tree static library at
`build/libjcccol.a` is the same artifact that ends up in the release
archive. A typical link line:

```bash
clang -o myprogram myprogram.c -I/path/to/libjcccol/include \
      -L/path/to/libjcccol/build -ljcccol
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
