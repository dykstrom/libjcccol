# Local builds and cross-platform verification

`make` and `make test` build and test the host platform. For the build system
itself — targets, flags, dist layout — see
[`../ARCHITECTURE.md`](../ARCHITECTURE.md).

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

See [`ci.md`](ci.md) for which of these platforms CI builds, and when.
