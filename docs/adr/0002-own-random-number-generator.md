# 0002. Own random number generator for col_rnd

*2026-10-09*

## Context

COL needs `rnd()`, which returns a number in [0, 1), and `randomize(seed)`,
which makes a run repeatable. libjccbas implements BASIC's RND with libc
`srand` and `rand`. On Windows, `RAND_MAX` is 32767, so `rand` gives only
32768 distinct values. Each C library also uses its own algorithm, so one
seed gives a different sequence on macOS, Linux and Windows. The
alternatives were libc `rand` as in libjccbas, or our own generator with or
without a seed function.

## Decision

We will implement xoshiro256** in `src/core.c`, and splitmix64 will turn
the seed into its four words of state. `col_randomize_i64` sets the seed.
If a program calls `col_rnd` first, that call seeds the generator from
`col_millis()`.

## Consequences

A seed gives the same sequence on every platform, so a seeded COL program
behaves the same on every OS, and tests can compare exact values.
`tests/test_core.c` checks the first three values for seed 42 on every CI
platform. Every value is a multiple of 2^-53, so the range has far more
distinct values than `rand` gives on Windows.

The sequence for each seed is now part of the public behaviour. A change of
algorithm or seeding must update the fixed values in `tests/test_core.c`,
and needs a new ADR. libjcccol and libjccbas give different sequences for
the same seed. The generator state is one global variable with no locking,
which is safe only because COL programs are single-threaded.
