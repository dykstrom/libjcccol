# Continuous integration

`.github/workflows/build.yml` builds and tests the platform matrix. For the
tag-triggered release flow in `release.yml`, see
[`../ARCHITECTURE.md`](../ARCHITECTURE.md).

| Event | Builds? |
|---|---|
| Push to any branch | Yes |
| Pull request from a fork into `main` or `dev` | Yes |
| Pull request from a fork into any other branch | No |
| Pull request from a branch in this repo | No — reported as skipped |
| `workflow_dispatch` | Yes |

`push` carries no `branches:` filter, so every branch builds on push. A
same-repo pull request would only repeat the build its own push already ran,
so the job is gated on the head repository differing from
`github.repository`. Fork pull requests produce no push event in this repo,
which makes them the only `pull_request` events that build, and `on:`
restricts those to the two long-lived branches.

That gate is a job-level `if:`, not a trigger filter — the `on:` block has no
fork predicate. Two consequences:

- A same-repo pull request still creates the check and reports it as skipped,
  rather than not creating it. GitHub counts a skipped job as successful, so
  the check stays safe to mark required in branch protection.
- The job deliberately carries no `name:`. A skipped job never expands its
  matrix, so a name built from `${{ matrix.platform }}` would appear
  unrendered in the checks list. Letting GitHub derive the name from the job
  id gives `build (linux-arm64)` when the matrix expands and plain `build`
  when it does not. `platform` is the only top-level matrix key for the same
  reason — the derived name lists top-level keys only, so everything else
  stays under `include:`. `jcc` uses this arrangement in its own workflows.
- Fork builds get a read-only `GITHUB_TOKEN` and no repository secrets. The
  current steps need neither.

## Superseded runs

A workflow-level `concurrency` group cancels an in-flight run when a newer
one starts for the same ref, so pushing twice in quick succession leaves only
the second building. The group key falls back to `github.ref`, but uses
`github.event.pull_request.head.label` when present: a pull request event
reports `refs/pull/N/merge` rather than its source branch, which would
otherwise put a fork PR's runs in a group of their own each time.

This applies to `build.yml` only. `release.yml` declares no concurrency
group, so a tag build is never cancelled by a later one.

See [`platforms.md`](platforms.md) for the six platforms the matrix covers.
