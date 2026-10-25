# Public API

Public functions are declared in headers under `include/jcccol/`. The
umbrella header `include/jcccol.h` includes every module header, so a caller
can write `#include <jcccol.h>` and get the whole API.

The rules that keep the API compatible with JCC are in
[`../architecture/public-api.md`](../architecture/public-api.md).

## Symbol naming

Every exported symbol follows the scheme below. The one exception is the
vendored garbage collector, which keeps its upstream `jcc_gc_*` prefix. See
[`vendored-gc.md`](vendored-gc.md).

**The `col_` prefix.** Every exported symbol starts with `col_`. A static
archive's symbols are visible to the whole link. Plain names like `eof`,
`substr` or `readln` could collide with libc, or with the user's own C code,
in the final link of a COL program.

**Signature mangling.** COL overloads functions by arity and parameter type,
so one COL name can need several exported functions. The exported name is:

```
col_<name>[_<type>...]
```

`<name>` is the COL function name. Each `<type>` is a token for one
parameter type, in declaration order:

| COL type | Token |
| --- | --- |
| string | `str` |
| integer | `i64` |
| i128 | `i128` |
| float | `f64` |
| boolean | `bool` |

Three rules make the mapping mechanical:

- The name encodes parameter types only, in declaration order. It does not
  encode the return type.
- A function with no parameters has no suffix: `col_millis`, not
  `col_millis_void`.
- The source string is the first parameter of every string function. This
  leaves room for a method-call syntax where `"hello".indexof("ll")` means
  `indexof("hello", "ll")`.

Examples:

| COL signature | Exported symbol |
| --- | --- |
| `millis()` | `col_millis` |
| `readln()` | `col_readln` |
| `concat(string, string)` | `col_concat_str_str` |
| `substr(string, integer, integer)` | `col_substr_str_i64_i64` |
| `indexof(string, string)` | `col_indexof_str_str` |
| `string(i128)` | `col_string_i128` |
| `string(boolean)` | `col_string_bool` |

The scheme follows libjccbas (`add_Str_Str`, `instr_I64_Str_Str`) but uses
lower-case type tokens. In libjccbas, the leading `I64` of
`instr_I64_Str_Str` is a parameter of INSTR's three-argument form, not a
return type. [ADR 0001](../adr/0001-col-symbol-naming-scheme.md) records
the decision and the alternatives.

## Other naming conventions

- Header guards use the `JCCCOL_<MODULE>_H` pattern. Vendored files keep
  their upstream guards.
- Internal and `static` functions have no `col_` prefix. The prefix marks
  the exported API only.

## Adding a new public function

1. Derive the exported name from the scheme above. JCC emits calls to this
   name directly, so the name is the whole contract.
2. Add the declaration to the right header under `include/jcccol/`, or
   create a new module header there. Include doc comments. Consider
   platform differences up front.
3. Implement it in `src/`. Use `#ifdef _WIN32` for Windows and POSIX APIs
   for macOS and Linux. See [`platforms.md`](platforms.md).
4. If you created a new module header, add it to the umbrella header
   `include/jcccol.h`.
5. Add tests in `tests/test_<module>.c` with the macros in
   `tests/test_framework.h`. Cover platform behavior and edge cases.
6. Verify with `make clean && make test`. All warnings are errors.
