# 0000. Record architecture decisions

*2026-08-07*

## Context

libjcccol's durable context is split by kind: `docs/system/` describes what
the code does today, `docs/architecture/` states the rules it must follow,
and `docs/reference/` holds long-form rationale. None of those records *why*
a rule was chosen over the alternatives that were on the table. Without that,
a future contributor rediscovering an old choice cannot tell a deliberate
trade-off from an accident, and re-litigates it. `docs/ARCHITECTURE.md`
carries an "Open Design Decisions" section for choices still unsettled, but
it has nowhere to send them once they are settled.

## Decision

We will record architecturally significant decisions as ADRs in `docs/adr/`,
following Michael Nygard's format slimmed to three sections: Context,
Decision, Consequences. Files are named `<NNNN>-<short-kebab-title>.md` with
a strictly sequential four-digit number, one decision per file. A decision is
significant enough to record when it affects the system's structure, a
non-functional characteristic, a foundational dependency, a public interface,
or a construction technique reused across the codebase — and when there was a
real choice between alternatives.

## Consequences

ADRs are immutable once shipped, meaning committed, pushed, or already relied
on by other work. A later change of course is a new ADR that supersedes the
old one; the only permitted edit to a shipped ADR is adding a
`> Superseded by <NNNN>.` line under its title. Numbers are never reused or
renumbered.

There is no `Status` field. An ADR in the repo is accepted by definition; one
still being drafted has not been committed yet.

Decisions below the bar — localized to one module, or the conventional
default — do not get an ADR. They belong in `docs/system/` or
`docs/architecture/`, which is where most durable knowledge lands.

This record is numbered 0000 rather than 0001 so that the first substantive
decision starts the sequence at 0001. `docs/adr/README.md` documents the same
conventions in operational form for anyone adding a record.
