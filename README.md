# Dotenv: Single Header Library for C

This library is currently a WIP and not ready for production.

```sh
gcc -g dotenv_test.c -o dotenv_test

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

