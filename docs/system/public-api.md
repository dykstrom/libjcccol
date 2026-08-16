# Public API

## Adding a new public function

1. Derive the exported symbol name from the scheme in
   [`../ARCHITECTURE.md`](../ARCHITECTURE.md#symbol-naming): `col_` prefix,
   then one lower-case token per parameter type in declaration order
   (`str`, `i64`, `f64`, `bool`), or no suffix at all if the function takes
   no arguments. JCC emits calls to this name directly, so getting it right
   is the whole contract.
2. Add it to the appropriate header under `include/jcccol/`, or create a new
   module header there. Include doc comments. Consider platform differences
   upfront.
3. Implement it in `src/` — `#ifdef _WIN32` for Windows, POSIX APIs for
   macOS and Linux.
4. If you created a new module header, add it to the umbrella header
   `include/jcccol.h`.
5. Add tests in `tests/test_<module>.c` using the framework macros in
   `tests/test_framework.h`. Cover cross-platform behavior and edge cases.
6. Verify with `make clean && make test`. All warnings are errors.

Naming, header-guard, and platform-abstraction conventions are documented in
[`../ARCHITECTURE.md`](../ARCHITECTURE.md#code-organization); this file covers
only the procedure.
