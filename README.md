# Dotenv: Single Header Library for C

**Pronounced**: "dotenv-cshell", stands for Dotenv C Single Header Library.

### Yet Another Dotenv?

Find general information about the dotenv pattern here:

https://www.dotenv.org/

Doesn't every language have a dotenv parsing library? Well, yes. And they exist
for C as well, but at the time of writing there wasn't an option that fit my
requirements well. Here are the goals of this project:

- Distributable as a single header file with zero dependencies.
- Removes quotes from strings.
- Decent test coverage.
- Compatible with C99 standard: `gcc -std=c99`.
- Simple enough not to require a makefile or build system, even for tests.
- Low memory footprint; doesn't need to read the entire file at once.

This library is currently a WIP and not ready for production.

### Platforms

This library is written in standard C and should be compatible with any compiler
that supports C99. That being said, the details of how environment variables
work on your platform may vary and OS specific shims are (currently) not part
of the library (Windows). This may change in a future version, and you can
provide a shim for `setenv()`.

- **macOS** (tested, works)
    - Apple clang version 14.0.0 (clang-1400.0.29.202) / macOS 12.6.1 **ok**
- **Linux** (tested, works)
    - gcc version 10.3.1 20210424 (Alpine 10.3.1_git20210424) **ok**
    - chibicc (version ? Alpine Linux v3.14)
- **UNIX/BSD** (untested, but should work)
- **Windows** (untested, requires `setenv()` shim)

### Installation

Add the `dotenv.h` file to your codebase.

In *one file only* define `DOTENV_IMPL`. For example, in your main file:

```c
#define DOTENV_IMPL
#include "dotenv.h"

int error = dotenv_load_from_path(".env");
if (error > 0) {
    printf("Error loading dotenv: %s\n", dotenv_strerror(error));
    return error;
}
```

Unlike many dotenv libraries that expect a path to a directory, `dotenv-cshl`
expects a path to the file itself. After years of using `.env` files in projects,
experience tells me that it's better for the programmer to explicitly set
the location of the file; configuration not convention, but also fewer surprises.

Environment variables are only loaded for the current process (by design), so
this is not useful for setting system-wide environment variables.

### Build and Run Tests

The project is so simple you don't need a makefile or build system.

- Tests are in the `test.c` file.
- A simple command line utility is in the `main.c` file.

The utility program expects a path to a `.env` file as the first argument. It
reads the contents into memory and then list all environment variables.
(Warning: if any environment variables contain secrets, these will be
logged to stdout by this program!) This is an easy way to test that `dotenv-cshl`
is reading your file correctly.

```sh
# without debug output
gcc test.c -o test

# with debug output
gcc -g test.c -o test -DDOTENV_DEBUG
```

### Text Encoding

Dotenv-cshl does minimal validation of the .env file. By default, it supports
ASCII 7-bit/latin1 and UTF-8 encoding, with or without BOM. If the .env file
is UTF-16 or UTF-32 with BOM, the library will return an error code during parse.
(This is equally true for any file with null bytes.)

This policy provides some sensible guardrails without adding a lot of overhead,
and it's expected that you can control the contents of the .env file. If you're
sure you don't care, `#define DOTENV_DISABLE_UTF_GUARDS`.

### Known Bugs & Issues
- Valgrind has "still reachable" warnings when linking with musl libc.

### Support for Small C Compilers

- `chibicc`: 01-14-2022 tested/working. On Alpine, you need to include "stdarg.h"
in your build command. ([Alpine support](https://github.com/matthew-macgregor/chibicc/tree/alpine)
is in my fork of `chibicc`.)

```sh
./chibicc -include stdarg.h main.c -o main.chibi
```

### To Do

- Document compile-time options.
- Additional tests, static analysis, valgrind.
- Windows support shim and testing.
- Linux testing.
- UNIX/BSD testing.