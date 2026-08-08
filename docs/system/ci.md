# Continuous integration

`.github/workflows/build.yml` builds and tests the platform matrix. For the
tag-triggered release flow in `release.yml`, see
[`../ARCHITECTURE.md`](../ARCHITECTURE.md).

| Event | Builds? |
|---|---|
| Push to any branch | Yes |
| Pull request from a fork | Yes |
| Pull request from a branch in this repo | No — reported as skipped |
| `workflow_dispatch` | Yes |

Neither `push` nor `pull_request` carries a `branches:` filter, so both match
every branch. A same-repo pull request would only repeat the build its own
push already ran, so the job is gated on the head repository differing from
`github.repository`. Fork pull requests produce no push event in this repo,
which makes them the only `pull_request` events that build.

That gate is a job-level `if:`, not a trigger filter — the `on:` block has no
fork predicate. Two consequences:

- A same-repo pull request still creates the check and reports it as skipped,
  rather than not creating it. GitHub counts a skipped job as successful, so
  the check stays safe to mark required in branch protection.
- Fork builds get a read-only `GITHUB_TOKEN` and no repository secrets. The
  current steps need neither.

See [`platforms.md`](platforms.md) for the six platforms the matrix covers.
