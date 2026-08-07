# Platforms

Six supported platforms, all built and released by CI:

| Platform | Runner | Toolchain |
|---|---|---|
| macOS arm64 | `macos-latest` | Xcode Clang |
| macOS x86_64 | `macos-15-intel` | Xcode Clang |
| Linux x86_64 | `ubuntu-latest` | apt `clang llvm` |
| Linux arm64 | `ubuntu-24.04-arm` | apt `clang llvm` |
| Windows x86_64 | `windows-latest` | MSYS2 MINGW64 |
| Windows arm64 | `windows-11-arm` | MSYS2 CLANGARM64 |

64-bit only; 32-bit is out of scope. There is no explicit minimum OS or libc
version — the supported baseline is whatever those runner images and current
MSYS2 packages provide.

Platform splits live in the implementation files, never in the public headers:
`src/core.c` branches on `#ifdef _WIN32` (Win32 API) versus `#else` (POSIX).
The POSIX branch requires `#define _POSIX_C_SOURCE 200809L` before any system
header, so glibc exposes `clock_gettime`/`CLOCK_REALTIME` under `-std=c11`.
macOS headers are permissive and compile without it; Linux is where the
omission surfaces.

The Makefile carries two platform adaptations: `EXE := .exe` when `uname -s`
contains `MINGW` or `OS=Windows_NT` is set, baked into the test-binary pattern
rule; and `DIST_OS`, which normalizes `uname -s` to `macos`/`linux`/`windows`
so a local `make dist` produces the same archive names CI does.

See [`../ARCHITECTURE.md`](../ARCHITECTURE.md#platform-handling) for the
abstraction principle this follows.

## Still to document

- When a function has no Win32 equivalent, what is the convention — stub it,
  return an error, or keep it off the public API entirely? No precedent yet;
  `col_millis()` has an implementation on both sides. (The vendored
  collector needs no platform split at all — `src/jcc_gc.c` contains no
  `#ifdef _WIN32`.)
