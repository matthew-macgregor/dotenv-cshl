#include <string.h>
#include <stdio.h>
#include <assert.h>

#define DOTENV_IMPL
#include "dotenv.h"

// Name            FG  BG
// ----------------------
// Black           30  40
// Red             31  41
#define CON_RED "\033[0;31m"
// Green           32  42
#define CON_GREEN "\033[0;32m"
// Yellow          33  43
#define CON_YELLOW "\033[0;33m"
// Blue            34  44
#define CON_BLUE "\033[0;34m"
// Magenta         35  45
#define CON_MAGENTA "\033[0;35m"
// Cyan            36  46
// White           37  47
// Bright Black    90  100
// Bright Red      91  101
// Bright Green    92  102
// Bright Yellow   93  103
// Bright Blue     94  104
// Bright Magenta  95  105
// Bright Cyan     96  106
// Bright White    97  107
#define CON_RESET "\033[0m"

char *dotenv_contents =
    "\xEF\xBB\xBF BOM=value after BOM\n" // BOM
    "\x1ELINE_FEED=line feed\n" // ignore line feed, unprintable characters 
    "\n# This is just a comment\n"
    "ZERO=VALUE_WITHOUT_ANY_SPACES # This is just a comment\n"
    "ONE=value with spaces  \n"
    "#TWO=\"blah blah blah\"\n"
    "THREE='value with single quotes and a comment' # comment happens here\n\n"
    "### Comment\n"
    "FOUR=https://en.wikipedia.org/wiki/C_(programming_language)\n"
    "FIVE=contains-an-equals-sign=123\n"
    "UTF_8=😀😀😀😀😀\n"
    "1NUM=Starts with number\n"
    "\n"
;

const int dotenv_expectations_len = 20;
char *dotenv_expectations[dotenv_expectations_len] = {
    "BOM", "value after BOM",
    "LINE_FEED", "line feed",
    "ZERO", "VALUE_WITHOUT_ANY_SPACES",
    "ONE", "value with spaces",
    "TWO", NULL,
    "THREE", "value with single quotes and a comment",
    "FOUR", "https://en.wikipedia.org/wiki/C_(programming_language)",
    "FIVE", "contains-an-equals-sign=123",
    "UTF_8", "😀😀😀😀😀",
    "1NUM", "Starts with number" // TODO: should this be legal?
};

void assert_str_equals(char *key, char *expect_value) {
    char *value = getenv(key);
    printf(
        CON_GREEN "Testing %s%s %s=>%s <%s> == <%s> ", 
        CON_YELLOW, key, 
        CON_MAGENTA, CON_BLUE,
        expect_value, value
    );
    if (value == NULL) {
        printf(CON_RED);
        assert(expect_value == value);
        printf(CON_RESET);
    } else {
        printf(CON_RED);
        assert(strcmp(value, expect_value) == 0);
        printf(CON_RESET);
    }
    printf(CON_GREEN "Ok.\n" CON_RESET);
}

int write_test_env_file(const char* filename) {
    FILE *fp = fopen(filename, "w+");
    int retval = 0;
    if (fp) {
        fputs(dotenv_contents, fp);
    } else {
        retval = 1;
    }

    fclose(fp);
    return retval;
}

int main(int argc, char **argv) {
    int error;

    error = write_test_env_file(".env");
    if (error > 0) {
        printf("Failed to open dotenv file for testing: %s\n", dotenv_strerror(error));
        return error;
    }

    error = dotenv_load_from_path(".env");
    if (error > 0) {
        printf("Error loading dotenv: %s\n", dotenv_strerror(error));
        return error;
    }

    for (int i = 0; i < dotenv_expectations_len; i += 2) {
        char *key = dotenv_expectations[i];
        if (key != NULL) {
            assert_str_equals(key, dotenv_expectations[i + 1]);
        }
    }
}
