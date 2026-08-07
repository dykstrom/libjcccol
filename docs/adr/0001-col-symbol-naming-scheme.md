# 0001. COL symbol naming scheme

*2026-08-07*

## Context

JCC-emitted LLVM IR references libjcccol symbols by name and the linker
resolves them from `libjcccol.a` at COL link time, so the exported symbol name
is the ABI contract — JCC never compiles against the headers. Until now the
archive exported one symbol, `millis`, which was safe only because it was
alone; a static archive's symbols are visible to the whole link, and the string
functions COL needs next (`eof`, `substr`, `readln`, `concat`, `indexof`) would
risk colliding with libc or with a COL programmer's own C code. The repo's
first architecture review left this open as "symbol prefix policy", to be
settled before the second public function landed, offering three options: keep
names unprefixed and trust link-time isolation; prefix everything `col_`; or
keep unprefixed names in the headers as static-inline wrappers around prefixed
link symbols. The wrapper option buys nothing here precisely because JCC
resolves by symbol name rather than through the headers. A prefix alone is also
insufficient, because COL overloads by arity and parameter type, so one COL
name can require several exported implementations.

## Decision

We will prefix every exported libjcccol symbol `col_`, and mangle overloadable
functions as `col_<name>[_<type>...]`, using lower-case parameter-type tokens
(`str`, `i64`, `f64`, `bool`) in declaration order. This follows libjccbas's
scheme (`add_Str_Str`, `instr_I64_Str_Str`) but lower-cases the tokens, on the
view that lower-case is where libjccbas should itself migrate. The vendored
garbage collector is the sole exception and keeps its upstream `jcc_gc_*`
names, because JCC's language-agnostic `RuntimeGcCodeGenerator` emits those
names for both BASIC and COL.

## Consequences

Symbol names now follow mechanically from a COL signature, so the string
runtime functions can be named without further deliberation
(`col_concat_str_str`, `col_substr_str_i64_i64`, `col_indexof_str_str`,
`col_string_i64`). Collisions with libc and with user C code are structurally
prevented rather than avoided case by case.

Renaming `millis` to `col_millis` is an ABI break. JCC pins a libjcccol
version, so nothing breaks until that pin moves and `JF_MILLIS`'s
`ExternalFunction` string is updated in the same step.

Because only parameter types are encoded, two COL functions that differ solely
in return type cannot both be exported. COL has no such pair today, and adding
one would require extending this scheme.

The lower-case tokens are a knowing divergence from libjccbas, which stays on
`Str`/`I64` until it migrates. Until then the two libraries mangle the same
concept differently, and anyone reading both needs to know why.
