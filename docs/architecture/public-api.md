# Public API

Operational rules in MUST voice. Each rule cites its source. The naming
scheme itself is described in [`../system/public-api.md`](../system/public-api.md).

## Naming

- Every exported symbol MUST start with `col_` and MUST be mangled as
  `col_<name>[_<type>...]`, with lower-case parameter-type tokens in
  declaration order.
  *Source: [ADR 0001](../adr/0001-col-symbol-naming-scheme.md).*
- The vendored garbage collector MUST keep its upstream `jcc_gc_*` names.
  *Source: [ADR 0001](../adr/0001-col-symbol-naming-scheme.md). JCC's
  `RuntimeGcCodeGenerator` emits those names for both BASIC and COL.*
- Two exported functions MUST NOT differ only in return type, because the
  scheme does not encode the return type.
  *Source: [ADR 0001](../adr/0001-col-symbol-naming-scheme.md).*

## Compatibility with JCC

JCC-emitted LLVM IR calls libjcccol functions by symbol name, and JCC does
not compile against the headers. Adding an exported symbol is therefore
compatible, and JCC ignores symbols it does not reference. See
[`../system/jcc-integration.md`](../system/jcc-integration.md).

- Renaming or removing an exported symbol MUST be treated as an ABI break,
  and the release notes MUST mark it **Breaking:**.
  *Source: JCC links by symbol name. The release-notes format is in
  [`../system/release.md`](../system/release.md).*
