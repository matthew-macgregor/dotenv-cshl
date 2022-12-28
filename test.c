#include <string.h>
#include <stdio.h>
#include <assert.h>

#define DOTENV_IMPL
#include "dotenv.h"
#include "colors.h"

#define DOTENV_EXPECTATIONS_LEN 22

const char *dotenv_contents =
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
    "😀😀😀=laughing\n"
    "\n"
;

const char *dotenv_expectations[DOTENV_EXPECTATIONS_LEN] = {
    "BOM", "value after BOM",
    "LINE_FEED", "line feed",
    "ZERO", "VALUE_WITHOUT_ANY_SPACES",
    "ONE", "value with spaces",
    "TWO", NULL,
    "THREE", "value with single quotes and a comment",
    "FOUR", "https://en.wikipedia.org/wiki/C_(programming_language)",
    "FIVE", "contains-an-equals-sign=123",
    "UTF_8", "😀😀😀😀😀", // TODO: should this be legal?
    "1NUM", "Starts with number", // ?
    "😀😀😀", "laughing" // ?
};

void assert_str_equals(const char *key, const char *expect_value) {
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
        // UTF-32 LE
        // fprintf(fp, "%c%c%c%c",  0xFF, 0xFE, 0x00, 0x00);
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

    for (int i = 0; i < DOTENV_EXPECTATIONS_LEN; i += 2) {
        const char *key = dotenv_expectations[i];
        if (key != NULL) {
            assert_str_equals(key, dotenv_expectations[i + 1]);
        }
    }
}
