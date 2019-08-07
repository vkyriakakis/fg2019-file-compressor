#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void reportError(char *message, char *filename, int line) {
	fprintf(stderr, "%s:%d: ", filename, line);
	perror(message);
}