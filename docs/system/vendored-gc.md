# Vendored garbage collector

`libjcccol` ships the JCC runtime garbage collector, a precise mark-sweep
collector with compiler-managed roots. COL needs it because strings are
COL's first heap type.

**`libjccbas` is the canonical copy.** `libjcccol` vendors an identical
one, as `jcc_gc.h`'s own header comment states. Fixes go upstream first,
then get re-vendored here — never the other way round.

## What was copied, and from where

| Upstream (`dykstrom/libjccbas`) | Here |
| --- | --- |
| `main/inc/jcc_gc.h` | `include/jcccol/jcc_gc.h` |
| `main/src/jcc_gc.c` | `src/jcc_gc.c` |

Taken from commit `abe52c2850a4e3076f0b121fc0e3bfe3afac9450` (2026-07-22).

The copy is **verbatim** — GPL file header, `JCC_GC_H_` include guard,
formatting and all. That is deliberate: it makes re-vendoring a `cp`, and
makes drift a one-line `diff`. It is also why this provenance note lives
in `docs/` rather than in a comment at the top of the files.

### The one permitted divergence

`src/jcc_gc.c` line 25:

```c
#include "jcccol/jcc_gc.h"    /* upstream: #include "jcc_gc.h" */
```

The header sits one directory deeper here than it does upstream. Nothing
else may differ.

## Symbols and the header

The collector keeps its upstream `jcc_gc_*` prefix rather than taking
libjcccol's `col_` prefix (see
[`../ARCHITECTURE.md`](../ARCHITECTURE.md#symbol-naming)). JCC's
`RuntimeGcCodeGenerator` emits calls to these names for both BASIC and
COL, so the names are fixed by the compiler, not by this library.

`jcc_gc.h` is a **public** header: it ships in the dist bundle and is
included from the umbrella `jcccol.h`. The `jcc_gc_*` symbols are exported
from `libjcccol.a` whether or not the header ships, so shipping it
documents a contract that exists regardless.

## Tests

`tests/test_jcc_gc.c` ports all ten scenarios from libjccbas's
`test/src/test_jcc_gc.c`, translated to this repo's `TEST`/`RUN_TEST`/
`ASSERT` framework. They exist so a bad re-vendor fails `make test` here
rather than surfacing as a miscompiled COL program.

Two ordering constraints:

- `debug_exit_stats` **must run last** — it calls `jcc_gc_shutdown()`,
  after which the `atexit`-installed handler is a no-op. It asserts on the
  exact `jcc_gc: exit: registered=N collections=M freed=K live=L` line
  that JCC's own integration tests match on.
- The file defines `_POSIX_C_SOURCE 200809L` at the very top, because
  `debug_exit_stats` calls `putenv`. `jcc_gc.c` itself needs no feature
  macro — it only uses `getenv`, which is C11.

## Re-vendoring

```bash
cp ~/Workspace/libjccbas/main/inc/jcc_gc.h include/jcccol/jcc_gc.h
cp ~/Workspace/libjccbas/main/src/jcc_gc.c src/jcc_gc.c

# reapply the include-path edit
perl -pi -e 's{#include "jcc_gc.h"}{#include "jcccol/jcc_gc.h"}' src/jcc_gc.c

# confirm nothing else moved: the header diff is empty, the .c diff is
# exactly the one include line
diff ~/Workspace/libjccbas/main/inc/jcc_gc.h include/jcccol/jcc_gc.h
diff ~/Workspace/libjccbas/main/src/jcc_gc.c src/jcc_gc.c

make clean && make test
```

Then update the commit hash recorded above, and re-port
`test/src/test_jcc_gc.c` if its scenarios changed.
