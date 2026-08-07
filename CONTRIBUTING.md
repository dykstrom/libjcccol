# Contributing to libjcccol

This document describes how the repository is organized into branches and
how changes make their way into a release.

For build and test commands, see [AGENTS.md](AGENTS.md).

## Branch model

libjcccol uses two long-lived branches:

- **`main`** — the stable release branch, and the default branch on GitHub.
  Every tagged release is cut from here. It should always build.
- **`dev`** — the active integration branch. All day-to-day work lands here
  first and is stabilized before the next release.

## Feature branches

Create a branch off `dev` for each change. Name it `type/short-description`,
where `type` is one of:

| Prefix | Used for |
|--------|----------|
| `feature/` | New functionality |
| `fix/` | Bug fixes |
| `docs/` | Documentation only |
| `ci/` | Build and CI changes |

For example: `fix/posix-feature-macro`.

## Landing a change

1. Open a pull request from your feature branch into `dev`.
2. Merge it as a **squash merge**, so `dev` keeps a single, clean commit per
   change.
3. Delete the feature branch after it is merged.

## Releases

At release time, `dev` is merged into `main` with a normal merge — not a
squash — so `main` keeps the individual commits from `dev`.

Cut the release from `main`:

```bash
make release NEW_VERSION=X.Y.Z
```

This runs `make clean && make test`, bumps the `VERSION` file, commits, and
creates an annotated `vX.Y.Z` tag. It does **not** push. Review with
`git show`, then publish:

```bash
git push --follow-tags origin main
```

The branch constraint is enforced in two places: `scripts/release.sh` refuses
to run off `master`/`main`, on a dirty working tree, or when the tag already
exists locally or on origin; and `release.yml`'s `verify-branch` job fails any
tag whose commit is not an ancestor of `origin/main` or `origin/master`.

`release.sh` is bash and supports macOS and Linux only. Windows users cutting
a release should run the equivalent git steps manually — see the header
comment in `scripts/release.sh`.

## Hotfixes

An urgent bug fix may be committed directly on `main` and then merged back
into `dev` to keep the two branches in sync. In practice this is rare —
libjcccol is a single-maintainer project, so almost everything flows through
`dev`.
