# JCC integration

[JCC](https://github.com/dykstrom/jcc) is a Java and Kotlin project built
with Maven. It consumes libjcccol through the per-platform archives on this
repo's GitHub Releases. JCC uses the same integration for
[`libjccbas`](https://github.com/dykstrom/libjccbas). It is configured in
JCC's `jcc-compiler/pom.xml`. The current reference is the
[`llvm-basic`](https://github.com/dykstrom/jcc/blob/llvm-basic/jcc-compiler/pom.xml)
branch.

## How JCC gets the library

1. A `v*` tag pushed to `main` triggers `release.yml`, which publishes a
   GitHub Release with one archive per platform. See
   [`release.md`](release.md).
2. JCC's `jcc-compiler` module pins a libjcccol version. At build time,
   Maven detects the OS and architecture and activates a per-platform
   profile. The profile sets `native.classifier` and `native.archive.type`.
3. Maven first looks for the archive in the local Maven repository at
   `~/.m2/repository/se/dykstrom/jcc/libjcccol/${version}/libjcccol-${version}-${classifier}.${type}`.
   If it is not there, Maven downloads it from
   `https://github.com/dykstrom/libjcccol/releases/download/v${version}/libjcccol-${version}-${classifier}.${type}`
   and installs it in the local repository with `mvn install:install-file`.
4. `maven-dependency-plugin:unpack` extracts the archive into JCC's
   `target/temp-extract/`. An antrun step then copies the `libjcccol.*`
   files, without their folders, into `target/`.
5. `maven-resources-plugin` copies `libjcccol.a` into JCC's `bin/`
   directory, so the library ships with the JCC distribution. It would also
   copy Windows `libjcccol.dll` files, but none exist today.

## Archive names

JCC expects archives named `libjcccol-${version}-${classifier}.${type}`.
`${version}` is the git tag without the leading `v`. `${classifier}` and
`${type}` are:

| OS | Architecture | Classifier | Archive type |
| --- | --- | --- | --- |
| Windows | x86_64 | `windows-x86_64` | `zip` |
| Windows | arm64 | `windows-arm64` | `zip` |
| macOS | x86_64 | `macos-x86_64` | `tar.gz` |
| macOS | arm64 | `macos-arm64` | `tar.gz` |
| Linux | x86_64 | `linux-x86_64` | `tar.gz` |
| Linux | arm64 | `linux-arm64` | `tar.gz` |

Both `make dist` and `release.yml` produce names in this format. See
[`build.md`](build.md) for `make dist`.

## What JCC uses from the archive

Each archive contains:

```
libjcccol-<version>-<classifier>/
├── lib/libjcccol.a
├── include/jcccol.h
├── include/jcccol/*.h
├── README.md
└── LICENSE
```

JCC uses only `lib/libjcccol.a`. Its antrun step copies `**/libjcccol.*`
with `flatten="true"`, so the folder layout inside the archive does not
matter. Only the presence of `libjcccol.a` does.

JCC does not use the headers. Its COL compiler emits LLVM IR that calls
libjcccol functions by symbol name, and the linker finds them in
`libjcccol.a` when it links a COL program. The archive ships the headers
for C programs and as API documentation. A change to a header alone
therefore does not affect JCC. The compatibility rules that follow from
this are in [`../architecture/public-api.md`](../architecture/public-api.md).
