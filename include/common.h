#ifndef MANO_COMMON_H
#define MANO_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#define MANO_VERSION "1.0.0"
#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 64

typedef enum {
    MANO_OK = 0,
    MANO_ERROR_LEXICAL,
    MANO_ERROR_SYNTAX,
    MANO_ERROR_SEMANTIC,
    MANO_ERROR_RUNTIME,
    MANO_ERROR_MEMORY,
    MANO_ERROR_IO,
    MANO_ERROR_COMPILER
} ManoError;

typedef struct {
    ManoError code;
    char message[512];
    int line;
    int column;
} ManoErrorInfo;

void mano_error_init(ManoErrorInfo *error);
void mano_error_set(ManoErrorInfo *error, ManoError code, const char *msg, int line, int column);
void mano_error_print(const ManoErrorInfo *error);

#endif
