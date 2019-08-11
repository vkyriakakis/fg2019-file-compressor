#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void reportErrorF(char *message, char *filename, const char *func, int line) {
    fprintf(stderr, "%s: %s,%d: ", filename, func, line);
    perror(message);
}