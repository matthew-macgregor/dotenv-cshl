#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "colors.h"

// Include the header as often as you'd like, but only define DOTENV_IMPL once.
#define DOTENV_IMPL
#include "dotenv.h"

extern char **environ;

int main(int argc, char** argv) {
    int error;
    char* path = NULL;

    if (argc > 1) {
        path = argv[1];
    } else {
        path = ".env";
    }

    error = dotenv_load_from_path(path);
    if (error > 0) {
        printf(CON_RED);
        printf("Error loading dotenv from '%s': %s\n", path, dotenv_strerror(error));
        printf(CON_RESET);
        return error;
    }

    for(char **current = environ; *current; current++) {
        puts(*current);
    }
    return EXIT_SUCCESS;
}