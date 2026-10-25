# Releases

The branch flow before a release is in
[`../../CONTRIBUTING.md`](../../CONTRIBUTING.md). This file describes how a
release is cut and published.

A release is a `v*` tag on a commit on `main` or `master`. Pushing the tag
triggers `.github/workflows/release.yml`. The workflow builds and tests the
six platforms in [`platforms.md`](platforms.md), runs `make dist` on each,
and uploads the archives to a GitHub Release. The archive names are listed
in [`jcc-integration.md`](jcc-integration.md). Nobody needs the `gh` CLI
locally to release.

The `VERSION` file at the repo root is the single source of truth for the
version. The Makefile reads it with `$(shell cat VERSION)`. Either the
release script or a feature change bumps it, as described below.

## Cutting a release on macOS or Linux

Start from a clean tree on `main`:

```bash
make release NEW_VERSION=X.Y.Z    # tests, bumps VERSION, commits, tags. Does not push
git show HEAD && git tag -n1 vX.Y.Z   # review
git push --follow-tags origin main    # publish. This triggers release.yml
```

`scripts/release.sh` takes one of two paths, depending on `VERSION`:

- **`VERSION` differs from the target.** The script writes the file,
  commits `Release vX.Y.Z`, and tags. That commit is made on `main`. `dev`
  gets it when `dev` is fast-forwarded to `main` after the release, as was
  done for v0.3.0.
- **`VERSION` already equals the target.** The script prints `VERSION
  already X.Y.Z — skipping bump commit` and creates only the tag. This
  happens when a feature change bumped `VERSION` and it reached `main`
  through `dev`. v0.2.0 was released this way.

To abort before pushing, run `git tag -d vX.Y.Z`. If the script created a
bump commit, also run `git reset --hard HEAD~1`. Do not run the reset on
the second path: there is no release commit, so the reset discards the last
real commit instead. `release.sh` prints the right abort commands at the
end of each run. See [`../../scripts/release.sh`](../../scripts/release.sh)
for the exact checks.

## Cutting a release on Windows

`release.sh` is bash and runs on macOS and Linux only. On Windows, run the
same git steps by hand:

```bash
echo X.Y.Z > VERSION
git commit -am "Release vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push --follow-tags
```

## Release notes

The `create-release` job in `release.yml` writes the release notes. They
hold a header, a What's Changed section that links the commit history, and
Platform Support. They have no installation or usage steps, because JCC
downloads and bundles the archive itself.

After the release is published, the maintainer expands What's Changed by
hand on the GitHub release page. It lists only user-visible changes, one
paragraph each with a bold lead-in. A change that breaks existing programs
or their linking starts with **Breaking:**. Releases 0.2.0 and 0.3.0 follow
this format.
