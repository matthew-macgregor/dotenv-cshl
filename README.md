# Dotenv: Single Header Library for C

**Pronounced**: "dotenv-cshell", stands for Dotenv C Single Header Library.

### Yet Another Dotenv?

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

### Build and Run Tests

The project is so simple you don't need a makefile or build system:

```sh
# without debug output
gcc dotenv_test.c -o dotenv_test

# with debug output
gcc -g dotenv_test.c -o dotenv_test -DDOTENV_DEBUG
```

### Text Encoding

Dotenv-cshl does minimal validation of the .env file. By default, it supports
ASCII 7-bit/latin1 and UTF-8 encoding, with or without BOM. If the .env file
is UTF-16 or UTF-32 with BOM, the library will return an error code during parse.
(This is equally true for any file with null bytes.)

This policy provides some sensible guardrails without adding a lot of overhead,
and it's expected that you can control the contents of the .env file. If you're
sure you don't care, `#define DOTENV_DISABLE_UTF_GUARDS`.

