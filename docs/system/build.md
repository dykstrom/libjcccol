# Build

The Makefile drives the build, the tests, packaging and releases. `make` and
`make test` build and test the host platform. The commands are listed in
[`../../AGENTS.md`](../../AGENTS.md).

## Makefile

- The Makefile uses Clang on every platform. `clang` is the default `CC`.
- It finds the `.c` files in `src/` and `tests/` by itself, with
  `$(wildcard ...)`.
- The compiler flags are `-Wall -Wextra -Werror -std=c11 -O2 -MMD -MP`.
  Warnings are errors. `-MMD -MP` writes a `.d` dependency file as part of
  each compile, so dependency tracking always matches the build.
- Test objects go in `obj/tests/`, so a test file cannot collide with a
  `src/` object of the same name.
- Compiling a test (`obj/tests/%.o`) and linking it (`build/%`) are separate
  rules, so a test can grow to several source files.
- Test binaries get the suffix in `EXE`: `.exe` on Windows, empty elsewhere.
  See [`platforms.md`](platforms.md) for how the Makefile detects Windows.
- Recipes print the full compiler and archiver commands. Only the status
  lines such as `Compiling: src/core.c` use a silent `@echo`. There is no
  `V=1` switch for verbose output. A 2026-05 review proposed one, assuming
  the recipes hid the `clang` command. They do not, so the switch was not
  added.

### Toolchain overrides

`CC`, `AR` and `RANLIB` can be overridden from the environment or the
command line, for example `make CC=gcc`. The Makefile uses
`ifeq ($(origin CC),default)` instead of `?=` for `CC` and `AR`, because make
predefines both (`cc` and `ar`). A plain `CC ?= clang` would leave `cc` in
place and build with gcc on Linux.

### Version

`VERSION` at the repo root holds the library version, and the Makefile reads
it into `$(VERSION)`. The version is used only for archive names and
`make version`. It is not passed to the compiler, and no C function returns
it. Add a `-DJCCCOL_VERSION_STRING=...` flag and a C function together when
a program first needs the version at run time.

### Lint

`make lint` runs [actionlint](https://github.com/rhysd/actionlint) over
`.github/workflows/` and [shellcheck](https://www.shellcheck.net/) over
`scripts/*.sh`. Each tool runs only if `command -v` finds it. Otherwise
`make lint` prints that it skipped the tool and does not fail. `make dist`
depends on `lint`, so a local `make dist` catches workflow and script errors
before a release. Install the tools with `brew install actionlint shellcheck`.

### Dist bundles

`make dist PLATFORM=<name> ARCHIVE=tar.gz|zip` stages a bundle in
`dist/libjcccol-<PLATFORM>/` and creates the archive. The bundle holds
`lib/`, `include/`, `README.md` and `LICENSE`. The Makefile defines this
layout in one place, so the workflows do not repeat it.

The default `PLATFORM` maps `uname -s` to `macos`, `linux` or `windows`, so a
local `make dist` produces the same archive names as CI. The names are listed
in [`jcc-integration.md`](jcc-integration.md).

## Tests

`tests/test_framework.h` is a small test framework written for this repo. It
provides the `TEST`, `RUN_TEST`, `ASSERT` and `ASSERT_MSG` macros and a
`test_sleep_ms` helper. Each `tests/test_<module>.c` builds into its own
binary, which returns 0 when all its tests pass. `make test` runs every
binary and stops at the first failure.

## Verifying Linux from macOS

macOS headers are permissive, so a missing feature-test macro compiles clean
on the host and fails only on glibc. Both such bugs found so far reached the
default branch this way. The gotchas in [`../../AGENTS.md`](../../AGENTS.md)
list the specific macros; this section is how to check them without pushing.

Linux runs locally through Docker on colima. **Ask the developer before
starting colima or a container.** Booting a VM on their machine is their
call, not the agent's — propose it and wait.

The images are maintained outside this repo; there is no Dockerfile here.
Both Linux CI architectures have one:

| Image | Matches CI job |
|---|---|
| `clang-docker:arm64` | `linux-arm64` |
| `clang-docker:amd64` | `linux-x86_64` |

The container bind-mounts `~/Workspace` at `/Workspace`, so this repo appears
at `/Workspace/libjcccol`. With the arm64 container running under the name
`clang-docker-arm64`, the full CI job is:

```sh
docker exec clang-docker-arm64 sh -c \
  'cd /Workspace/libjcccol && make clean && make && make test'
docker exec clang-docker-arm64 sh -c \
  'cd /Workspace/libjcccol && make dist PLATFORM=linux-arm64 ARCHIVE=tar.gz'
```

Two things to expect:

- The repo is bind-mounted and the container runs as root, so its builds write
  root-owned `build/`, `obj/`, and `dist/` into the host tree. Run `make clean`
  inside the container when finished, or those directories are left behind
  owned by root.
- The image carries neither actionlint nor shellcheck, so `make dist` skips
  linting there and reports it. Run `make lint` on the host instead.

To isolate a header-visibility question rather than build the whole tree,
compile a probe directly — this is what identified the `putenv` guard:

```sh
docker exec clang-docker-arm64 sh -c \
  'cd /tmp && printf "#include <stdlib.h>\nint main(void){static char b[]=\"K=V\";return putenv(b);}\n" > p.c
   clang -std=c11 -Werror -D_XOPEN_SOURCE=700 -c p.c -o /dev/null'
```

## Verifying Windows from macOS

Windows arm64 runs locally as the Parallels VM `Windows 11`. Its compiler is
llvm-mingw from winget: clang targeting `aarch64-w64-windows-gnu`. That is
not the MSYS2 CLANGARM64 toolchain that CI uses. There is no local Windows
x86_64.

**Ask the developer before starting Parallels Desktop or the VM.** Starting
either one is the developer's call, not the agent's. Propose it and wait.

The Makefile needs a POSIX shell, so run it from Git Bash, not `cmd`. The
`make` that Git Bash finds is GnuWin32's GNU Make 3.81, and it runs
`make test` with the real Makefile.

`prlctl exec` runs as SYSTEM by default, and SYSTEM does not have llvm-mingw
on its PATH. Pass `--current-user`. The macOS home folder appears as `Z:\`.
Copy the tree to a local folder first, so that the build does not write into
the host tree:

```sh
prlctl exec "Windows 11" --current-user cmd /c \
  'robocopy Z:\Workspace\libjcccol C:\Users\johan\libjcccol-win /MIR /XD .git build obj dist'
prlctl exec "Windows 11" --current-user cmd /c \
  '"C:\Program Files\Git\bin\bash.exe" -lc "cd /c/Users/johan/libjcccol-win && make clean && make test"'
prlctl exec "Windows 11" --current-user cmd /c 'rmdir /s /q C:\Users\johan\libjcccol-win'
```

Every `cmd` start in that VM prints "The system cannot find the path
specified." It comes from the VM's own setup and does not mean a step failed.

See [`ci.md`](ci.md) for which of these platforms CI builds, and when.
