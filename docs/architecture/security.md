# Security

Operational rules in MUST voice; cite the source (ADR, contract, incident) for
each.

- A `v*` tag MUST NOT produce a release unless its commit is an ancestor of
  `origin/main` or `origin/master`.
  *Source: `release.yml`, `verify-branch` job.*
- A release MUST NOT be cut from a branch other than `main`/`master`, from a
  dirty working tree, or with a tag that already exists locally or on origin.
  *Source: `scripts/release.sh` preflight checks.*
- `make clean && make test` MUST pass before the `VERSION` bump is committed.
  *Source: `scripts/release.sh`.*

## Still to document

- What MUST a public function do when given invalid input? Several functions
  take arguments now. [`../system/security.md`](../system/security.md)
  describes what they do: the integer and parsing paths accept any input,
  and a NULL string is a compiler bug. No rule states this yet.
- MUST the `create-release` job's `contents: write` permission stay scoped to
  that job, or is that incidental?
