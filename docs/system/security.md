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

The public API accepts no caller-supplied data: `millis(void)` takes no
arguments, so there is currently no input-validation surface. Revisit this file
when a function that takes arguments is added.

## Still to document

- Does JCC's Maven build perform any integrity check on the downloaded archive
  today, or does it trust the GitHub Releases URL outright?
- Is there a process for withdrawing or replacing a published release archive?
