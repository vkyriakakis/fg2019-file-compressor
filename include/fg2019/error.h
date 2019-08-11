#ifndef ERROR_GUARD

#define ERROR_GUARD

#define reportError(message)                                                   \
    (reportErrorF(message, __FILE__, __func__, __LINE__))
void reportErrorF(char *message, char *filename, const char *func, int line);

#endif
