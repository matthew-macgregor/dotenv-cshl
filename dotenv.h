#ifndef DOTENV_H
#define DOTENV_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Interface */
int dotenv_load_from_path(const char* path);
char *dotenv_strerr(int error);

#define DOTENV_STATUS_OK 0
#define DOTENV_STATUS_FREED 100
#define DOTENV_ERROR_ALLOC 101
#define DOTENV_ERROR_KEY_INVALID 102
#define DOTENV_ERROR_UNSUPPORTED_ENCODING 103

/* Implemenation */
#ifdef DOTENV_IMPL

#ifndef DOTENV_CHUNK_SZ
    #define DOTENV_CHUNK_SZ 512
#endif

#define DOTENV_START 0
#define DOTENV_KEY 1
#define DOTENV_VALUE 2
#define DOTENV_COMMENT 4
#define DOTENV_EQUAL 16
#define DOTENV_ENDL 32

#ifdef DOTENV_DEBUG
    #include <assert.h>
    #define DOTENV_DEBUG_PRINT(s, ...) printf(s, __VA_ARGS__)
#else
    #define DOTENV_DEBUG_PRINT(s, ...)
#endif

typedef struct dotenv_buffer {
    char *buffer;
    size_t size;
    int status;
} dotenv_buffer;

/**
 * @brief Allocates a dotenv_buffer structure. The caller is required to 
 * deallocate by calling dotenv_free_buffer().
 * 
 * @param buffer points to the structure to be initialized.
 * @param size n bytes of space to allocate.
 * @return int DOTENV_STATUS_OK or DOTENV_ERROR_ALLOC
 */
int dotenv_alloc_buffer(dotenv_buffer *buffer, size_t size) {
    buffer->buffer = (char*) calloc(size, 1);
    if (buffer->buffer != NULL) {
        buffer->status = DOTENV_STATUS_OK;
        buffer->size = size;
    } else {
        buffer->status = DOTENV_ERROR_ALLOC;
        buffer->size = 0;
    }

    return buffer->status;
}

/**
 * @brief Frees an allocated dotenv_buffer structure.
 * 
 * @param buffer  
 * @return int status code.
 */
int dotenv_free_buffer(dotenv_buffer *buffer) {
    assert(buffer->buffer != NULL);
    assert(buffer->size > 0);
    assert(buffer->status == DOTENV_STATUS_OK);

    free(buffer->buffer);
    buffer->buffer = NULL;
    buffer->size = 0;
    buffer->status = DOTENV_STATUS_FREED;

    return DOTENV_STATUS_FREED;
}

/**
 * @brief Expands the allocated storage of the dotenv_buffer struct.
 * 
 * On failure, this function will return an error status. In this case the
 * original buffer will still be allocated and the data preserved. It is safe
 * to free the object (and you should do so).
 * 
 * @param buffer 
 * @param expand_by n bytes to expand the allocated storage.
 * @return int status or error code.
 */
int dotenv_expand_buffer(dotenv_buffer *buffer, size_t expand_by) {
    assert(buffer->buffer != NULL);
    assert(buffer->size > 0);
    assert(buffer->status == DOTENV_STATUS_OK);

    DOTENV_DEBUG_PRINT("\nExpanding buffer by %zu\n", expand_by);

    char *tmp = (char*) realloc(buffer->buffer, buffer->size + expand_by);
    if (tmp == NULL) {
        // Buffer still points to the original memory block, which will need
        // to be freed later.
        buffer->status = DOTENV_ERROR_ALLOC;
        return DOTENV_ERROR_ALLOC;
    }

    buffer->buffer = tmp;
    // Zeroes the newly allocated memory to avoid a valgrind warning
    memset(buffer->buffer + buffer->size, 0, expand_by);
    buffer->size += expand_by;

    DOTENV_DEBUG_PRINT("Address of: %p\n", buffer->buffer);
    DOTENV_DEBUG_PRINT("Buffer size: %zu\n", buffer->size);

    return DOTENV_STATUS_OK;
}

/**
 * @brief Zeroes out the contents of the buffer and sets the size to 0 bytes,
 * but retains the allocated memory. Caller is responsible to call
 * dotenv_free_buffer().
 * 
 * @param buffer 
 */
static void dotenv_clear_buffer(dotenv_buffer *buffer) {
    assert(buffer->buffer != NULL);
    assert(buffer->size > 0);
    assert(buffer->status == DOTENV_STATUS_OK);
    memset(buffer->buffer, 0, buffer->size);
}

/**
 * @brief POSIX has guidelines for the contents of environment variables. This
 * function will return DOTENV_ERROR_KEY_INVALID if the argument contains illegal
 * characters and DOTENV_POSIX_STRICT is defined.
 * 
 * If DOTENV_POSIX_STRICT is not defined, any characters > ' ' are allowed.
 * 
 * @param str 
 * @return int DOTENV_ERROR_KEY_INVALID or DOTENV_STATUS_OK
 */
int dotenv_validate_key_strict(char *str) {
    int error = DOTENV_STATUS_OK;

    // First character cannot be a digit (POSIX)
    if (isdigit(str[0])) {
        error = DOTENV_ERROR_KEY_INVALID;
        return error;
    }

    while(*str != '\0') {
        if (
            !(  // POSIX defines: 3.231 Name a word consisting solely of 
                // underscores, digits, and alphabetics from the portable 
                // character set. The first character of a name is not a digit.
                (*str >= 'A' && *str <= 'Z') || 
                (*str >= 'a' && *str <= 'z') ||
                (*str >= '0' && *str <= '9') ||
                *str == '_'
            )
        ) {
            // illegal char
            error = DOTENV_ERROR_KEY_INVALID;
            break;
        }

        str++;
    }

    return error;
}

/**
 * @brief Sets the environment variable `key` to `value`. If DOTENV_POSIX_STRICT
 * is defined, also validates that the key contains legal characters.
 *     [a-zA-Z_]{1,}[a-zA-Z0-9_]{0,}
 * 
 * 
 * @param key 
 * @param value 
 * @param idx 
 * @return int DOTENV_STATUS_OK or an error code is setenv fails.
 */
int dotenv_setenv(dotenv_buffer *key, dotenv_buffer *value) {
    int error = DOTENV_STATUS_OK;

    // Enforce strict POSIX environment variable names
    #ifdef DOTENV_POSIX_STRICT
    error = dotenv_validate_key_strict(key->buffer);
    if (error != DOTENV_STATUS_OK) { return error; }
    #endif

    // okay to use strlen here, utf-8 doesn't matter
    if (strlen(key->buffer) > 0) {
        error = setenv(key->buffer, value->buffer, 1);
    }

    return error;
}

/**
 * @brief Trims leading and trailing whitespace and quotes from `str`, modifying
 * it in place.
 * 
 * @param str String to modify.
 */
static void dotenv_trim(char *str) {
    size_t orig_len = strlen(str);
    char *end;
    char *start = str;

    // Trim leading space
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // All spaces?
    if (*start == 0) {
        str[0] = '\0'; // Empty
        return;
    }

    // Trim starting single or double quotes
    while (*start == '\'' || *start == '\"') {
        start++;
        if (*start == 0) {
            str[0] = '\0';
            return;
        }
    }

    // Trim trailing space
    end = str + orig_len - 1;
    while (end > str && isspace((unsigned char) *end)) {
        end--;
    }

    // Trim end single or double quotes
    while (end > str && (*end == '\'' || *end == '\"')) {
        end--;
    }

    // Write new null terminator character
    end[1] = '\0';

    // No changes
    if (strlen(start) == strlen(str)) {
        return;
    }

    // Shifts the characters to the start of the buffer
    for (int i = 0;; i++) {
        str[i] = *start;
        start++;
        if (str[i] == '\0') {
            break;
        }
    }

    return;
}

const char *dotenv_strerror(int error) {
    // TODO: dotenv sets its error range > 100 to avoid the obvious POSIX codes
    // but I'm not absolutely sure that this is right and needs more research.
    if (error < 100) {
        return strerror(error);
    }

    switch (error) {
        case DOTENV_STATUS_OK:
            return "status ok";
        case DOTENV_STATUS_FREED:
            return "pointer has been freed";
        case DOTENV_ERROR_ALLOC:
            return "failed to allocate memory";
        case DOTENV_ERROR_KEY_INVALID:
            return "variable is not POSIX safe";
        case DOTENV_ERROR_UNSUPPORTED_ENCODING:
            return "unsupported text encoding detected";
        default:
            return "unknown error";
    }

    return "unexpected exit from dotenv_strerr";
}

/**
 * @brief Inspects the string for common text encodings.
 * 
 * @param str String to check.
 * @return int Number of bytes to skip. 
 */
static int dotenv_skip_bom(const char *str) {
    // UTF-8 0xEF 0xBB 0xBF
    // --------------------
    if ((unsigned char)str[0] == 0xEF && 
        (unsigned char)str[1] == 0xBB && 
        (unsigned char)str[2] == 0xBF) {
        return 3;
    }

    // If you're sure you don't want these guardrails in place, you can save a
    // few cycles by disabling UTF-16 and UTF-32 checks.
    #ifndef DOTENV_DISABLE_UTF_GUARDS
    // UTF-16 BE 0xFE OxFF
    // ------------------
    if ((unsigned char)str[0] == 0xFE && (unsigned char)str[1] == 0xFF) {
        // TODO: this works, but it's clumsy and a footgun.
        DOTENV_DEBUG_PRINT("Unsupported: %s detected\n", "UTF-16 BE BOM");
        return DOTENV_ERROR_UNSUPPORTED_ENCODING;
    }

    // UTF-16 LE 0xFF 0xFE
    // -------------------
    if (
        (unsigned char)str[0] == 0xFF && 
        (unsigned char)str[1] == 0xFE &&
        (unsigned char)str[2] != 0x00) {
        DOTENV_DEBUG_PRINT("Unsupported: %s detected\n", "UTF-16 LE BOM");
        return DOTENV_ERROR_UNSUPPORTED_ENCODING;
    }

    // UTF-32 BE 0x00 0x00 0xFE 0xFF
    // -----------------------------
    if ((unsigned char)str[0] == 0x00 &&
        (unsigned char)str[1] == 0x00 &&
        (unsigned char)str[2] == 0xFE &&
        (unsigned char)str[3] == 0xFF) {
        DOTENV_DEBUG_PRINT("Unsupported: %s detected\n", "UTF-32 BE BOM");
        return DOTENV_ERROR_UNSUPPORTED_ENCODING;
    }

    // UTF-32 LE 0xFF 0xFE 0x00 0x00
    // -----------------------------
    if ((unsigned char)str[0] == 0xFF &&
        (unsigned char)str[1] == 0xFE &&
        (unsigned char)str[2] == 0x00 &&
        (unsigned char)str[3] == 0x00) {
        DOTENV_DEBUG_PRINT("Unsupported: %s detected\n", "UTF-32 LE BOM");
        return DOTENV_ERROR_UNSUPPORTED_ENCODING;
    }
    #endif // DOTENV_DISABLE_UTF_GUARDS

    return 0;
}

/**
 * @brief Loads a dotenv file located at `path` into environment variables.
 * Note that `path` is a relative or absolute path to the .env file, not a
 * directory.
 * 
 * @param path relative or absolute path to the .env file.
 * @return int error if one occurs, or DOTENV_STATUS_OK (0)
 */
int dotenv_load_from_path(const char* path) {
    DOTENV_DEBUG_PRINT("Path: %s\n", path);

    FILE *fp = fopen(path, "r");
    if(fp == NULL) {
        return errno;
    }

    char chunk[DOTENV_CHUNK_SZ];
    memset(chunk, 0, DOTENV_CHUNK_SZ);
    dotenv_buffer key = { NULL, 0, DOTENV_STATUS_OK };
    dotenv_buffer value = { NULL, 0, DOTENV_STATUS_OK };

    dotenv_alloc_buffer(&key, DOTENV_CHUNK_SZ);
    dotenv_alloc_buffer(&value, DOTENV_CHUNK_SZ);

    DOTENV_DEBUG_PRINT("key size: %zu\n", key.size);
    DOTENV_DEBUG_PRINT("value size: %zu\n", key.size);

    int parse_mode = DOTENV_START;
    size_t idx = 0;
    int exit_status = DOTENV_STATUS_OK;

    // Processes stream up to the next newline character, end or len of stream
    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        char c = '\0';
        for (int i = 0; i < DOTENV_CHUNK_SZ; i++) {
            if (parse_mode == DOTENV_START) {
                int skip = dotenv_skip_bom(chunk);
                if (skip == DOTENV_ERROR_UNSUPPORTED_ENCODING) {
                    exit_status = skip;
                    goto cleanup;
                }
                i += skip;
                parse_mode = DOTENV_KEY;
            }

            c = chunk[i];
            if (c == '\0') {
                // Note: we may encounter a null byte without an endl at the
                // end of the file. Null byte always means stop processing this
                // chunk, but doesn't necessarily mean write the key/value.
                break;
            } else if (c == '#') {
                parse_mode = DOTENV_COMMENT;
            } else if (c == '=' && parse_mode != DOTENV_VALUE) {
                parse_mode = DOTENV_EQUAL;
            } else if (c == '\n') {
                parse_mode = DOTENV_ENDL;
            } else if (c < ' ' && c > '\0') {
                // Skip control characters and unprintables
                continue;
            }

            // Expand the buffer if we're out of space
            if (parse_mode == DOTENV_VALUE && idx >= value.size) {
                exit_status = dotenv_expand_buffer(&value, DOTENV_CHUNK_SZ);
            } else if (parse_mode == DOTENV_KEY && idx >= key.size) {
                exit_status = dotenv_expand_buffer(&key, DOTENV_CHUNK_SZ);
            }

            if (exit_status != DOTENV_STATUS_OK) {
                goto cleanup; // goto considered harmful, ha
            }

            if (parse_mode == DOTENV_ENDL) {
                dotenv_trim(key.buffer);
                dotenv_trim(value.buffer);
                value.buffer[idx + 1] = '\0';
                exit_status = dotenv_setenv(&key, &value);
                if (exit_status != DOTENV_STATUS_OK) {
                    goto cleanup;
                }
                dotenv_clear_buffer(&key);
                dotenv_clear_buffer(&value);

                idx = 0;
                parse_mode = DOTENV_KEY;
                break;
            } else if (parse_mode == DOTENV_KEY) {
                key.buffer[idx++] = c;
            } else if (parse_mode == DOTENV_VALUE) {
                value.buffer[idx++] = c;
            } else if (parse_mode == DOTENV_EQUAL) {
                key.buffer[idx + 1] = '\0';
                parse_mode = DOTENV_VALUE;
                idx = 0;
            }
        }
    }

    // If we reach the final null byte of the final chunk without a newline
    // character, we need to write the final key/value pair if they were
    // captured above.
    value.buffer[idx + 1] = '\0';
    dotenv_setenv(&key, &value);
    idx = 0;

    cleanup:
        dotenv_free_buffer(&key);
        dotenv_free_buffer(&value);
        fclose(fp);

    #ifdef DOTENV_DEBUG
    assert(key.buffer == NULL);
    assert(value.buffer == NULL);
    #endif

    return exit_status;
}

#endif /* DOTENV_IMPL */

#ifdef __cplusplus
}
#endif
#endif /* DOTENV_H */