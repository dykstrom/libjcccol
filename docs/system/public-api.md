# Public API

## Adding a new public function

1. Add it to the appropriate header under `include/jcccol/`, or create a new
   module header there. Include doc comments. Consider platform differences
   upfront.
2. Implement it in `src/` — `#ifdef _WIN32` for Windows, POSIX APIs for
   macOS and Linux.
3. If you created a new module header, add it to the umbrella header
   `include/jcccol.h`.
4. Add tests in `tests/test_<module>.c` using the framework macros in
   `tests/test_framework.h`. Cover cross-platform behavior and edge cases.
5. Verify with `make clean && make test`. All warnings are errors.

Naming, header-guard, and platform-abstraction conventions are documented in
[`../ARCHITECTURE.md`](../ARCHITECTURE.md#code-organization); this file covers
only the procedure.
