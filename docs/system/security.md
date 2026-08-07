# Security

The trust boundary is the release artifact, not the source tree. `release.yml`
publishes a per-platform archive to GitHub Releases; JCC's Maven build
downloads the archive matching its OS/arch, extracts `libjcccol.a`, and bundles
it under JCC's `bin/`.

**No integrity verification exists between publish and consumption.** The
archives carry no checksums and no signatures, and none are planned — this was
weighed and declined on 2026-08-07. Consumers trust the GitHub Releases URL
itself. Do not propose adding checksums as a routine improvement.

Anyone with push access to `origin` can cut a release: pushing a `v*` tag
triggers `release.yml`, and the only gate is the `verify-branch` job, which
rejects a tag whose commit is not an ancestor of `origin/main` or
`origin/master`. `contents: write` is scoped to the `create-release` job;
`build-and-test` runs with default permissions.

Third-party actions are pinned by major-version tag (`@v6`, `@v2`, `@v7`,
`@v8`, `@v3`) rather than commit SHA. This is deliberate — it keeps upstream
fixes flowing automatically, at the cost of trusting each action owner not to
move a published tag.

## Input surface

`col_millis(void)` takes no arguments and so has no input-validation surface.

The vendored garbage collector (`jcc_gc_*`, see
[`vendored-gc.md`](vendored-gc.md)) does take arguments, but it is a
**compiler-emitted contract, not a user-facing API**: JCC generates every
call. It validates nothing, by design — the header states the caller's
obligations (register a block before the next allocation, keep root slots
initialized, balance push/pop frames), and violating them is undefined
behavior. Treat a contract violation as a compiler bug, not as untrusted
input to be defended against.

Two environment variables read at `jcc_gc_init` are worth knowing about:

- `JCC_GC_DEBUG` — any non-empty value enables debug output, equivalent to
  passing the `JCC_GC_DEBUG` flag.
- `JCC_GC_LOG` — when debug is enabled, names a file the collector opens
  with `fopen(path, "a")` and writes to for the process's lifetime. The
  path is used as given, so a COL program's environment can direct these
  writes anywhere the process can write. It falls back to stdout if the
  open fails.

Both are inherited from `libjccbas` and are diagnostic features; neither is
gated on a build flag, so they are live in release builds.

## Still to document

- Does JCC's Maven build perform any integrity check on the downloaded archive
  today, or does it trust the GitHub Releases URL outright?
- Is there a process for withdrawing or replacing a published release archive?
