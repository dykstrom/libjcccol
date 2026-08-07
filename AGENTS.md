# AGENTS.md

Read [`CONTRIBUTING.md`](CONTRIBUTING.md) before making any changes — it
defines the branch model, how changes land, and how releases are cut.

## What is this

The standard library for the COL programming language, built as a static
library (`libjcccol.a`) in portable C11. Consumed by the
[JCC compiler](https://github.com/dykstrom/jcc): JCC's Maven build downloads
the matching per-platform release archive, extracts `libjcccol.a`, and
bundles it. JCC-emitted LLVM IR references libjcccol symbols by name; the
linker resolves them at COL link time. Targets macOS, Linux, and Windows
(MSYS2/MinGW).

## Stack

| Piece | Choice |
|-------|--------|
| Language | C11 (`-std=c11`), no third-party dependencies |
| Compiler | Clang by default on all platforms; `CC`/`AR` overridable |
| Build | GNU Make. Output is `build/libjcccol.a` |
| Testing | Custom header-only framework, `tests/test_framework.h`. One binary per `tests/test_*.c` |
| CI | GitHub Actions — `build.yml`, `release.yml` |
| Lint | `actionlint` + `shellcheck`, both optional |
| Versioning | `VERSION` file at the repo root, single source of truth |

## Directory index

| Path | What's there |
|------|-------------|
| `include/jcccol.h` | Umbrella header. A new module header must be added here to be part of the public API |
| `include/jcccol/` | Per-module public headers (`core.h`, `jcc_gc.h`, `strings.h`). Filenames are snake_case with no prefix; the **symbols** they declare are `col_`-prefixed and signature-mangled — see `docs/ARCHITECTURE.md` |
| `src/` | Implementations, one `.c` per module. Platform splits via `#ifdef _WIN32` |
| `tests/` | One `test_<module>.c` per module, plus `test_framework.h` |
| `scripts/` | `release.sh` — bumps `VERSION`, commits, tags. Bash; macOS/Linux only |
| `.github/workflows/` | `build.yml` (per-platform build + test), `release.yml` (dist bundles, publishes to Releases) |
| `build/`, `obj/`, `dist/` | Generated. Removed by `make clean` |
| `docs/` | Durable project context. Sub-folder layout below. `ARCHITECTURE.md` lives here — read it when orienting; it covers layout, build system, release process, JCC integration, and open design decisions. `system/public-api.md` is the checklist for adding a new public function |

```
docs/
├── system/         ← what the code does today (updated as code changes)
├── architecture/   ← what the system must do (updated when rules change)
├── adr/            ← architecture decisions (immutable once shipped)
├── reference/      ← long-form rationale (append-only)
└── working-notes/  ← research; NOT authoritative — rules live in architecture/ + adr/
```

## Commands

| What | Command |
|------|---------|
| Build | `make` → `build/libjcccol.a` |
| Test | `make test` — builds and runs every `build/test_*`, exits on first failure |
| Single test | `make build/test_core`, then `./build/test_core` |
| Version | `make version` |
| Lint | `make lint` — actionlint + shellcheck, silently skipped if not installed. `make dist` depends on it, so a machine without the tools packages without linting. Deliberate |
| Release | `make release NEW_VERSION=X.Y.Z` — runs tests, bumps `VERSION`, commits, tags. Does **not** push. See [`CONTRIBUTING.md`](CONTRIBUTING.md) |
| Dist bundle | `make dist PLATFORM=macos-arm64 ARCHIVE=tar.gz` — used by CI; bundle layout is defined in the Makefile, not the workflow YAML |

All compiler warnings are errors (`-Werror`). After editing `src/` or
`include/jcccol/`, verify with `make clean && make test`.

## Gotchas

- **`#define _POSIX_C_SOURCE 200809L` goes at the very top of every `.c` that
  uses POSIX-only symbols**, before any system header. Without it, glibc hides
  `clock_gettime`/`CLOCK_REALTIME` under `-std=c11`. macOS headers are
  permissive and won't catch the omission — Linux CI will.
- **The Makefile's `ifeq ($(origin CC),default)` is not stylistic.** Make
  pre-defines `CC=cc`, so a plain `CC ?= clang` is a no-op and the build
  silently falls through to gcc on Linux. Same for `AR`.
- **Step-level `shell:` in GitHub Actions can't be driven from a matrix
  field** — GHA parses `shell:` before matrix substitution. Hence the
  duplicated Windows / non-Windows step pairs in `release.yml`.
- **Test objects live under `obj/tests/`**, so a future `tests/core.c` won't
  collide with `src/core.c` at `obj/core.o`.
- **`src/jcc_gc.c` and `include/jcccol/jcc_gc.h` are vendored — do not edit
  them here.** `libjccbas` is the canonical copy; fixes go upstream and are
  re-vendored. The copy is verbatim apart from one `#include` line, so any
  local edit shows up as unexplained drift. See
  `docs/system/vendored-gc.md`.

## Working agreement

**Always make a plan before executing changes.** State what you're going to
change and why before any file-modifying tool call. For multi-step work,
write the plan out and get confirmation before proceeding. This applies to
every kind of change — new functionality, bug fixes, refactors, build-system
tweaks, docs.
