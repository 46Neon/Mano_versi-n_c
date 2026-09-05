#ifndef MANO_COMMON_H
#define MANO_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#define MANO_VERSION "2.0.0-functional"
#define MANO_ERROR_TEXT 512

typedef enum {
    MANO_OK = 0,
    MANO_ERR_ARGUMENT,
    MANO_ERR_MEMORY,
    MANO_ERR_IO,
    MANO_ERR_PARSE,
    MANO_ERR_DATA,
    MANO_ERR_TYPE,
    MANO_ERR_OVERFLOW,
    MANO_ERR_UNSUPPORTED,
    MANO_ERR_INTERNAL
} ManoStatus;

typedef struct {
    ManoStatus code;
    size_t line;
    size_t column;
    size_t row;
    char message[MANO_ERROR_TEXT];
} ManoError;

void mano_error_clear(ManoError *error);
void mano_error_set(ManoError *error, ManoStatus code, size_t line,
                    size_t column, size_t row, const char *message);
const char *mano_status_name(ManoStatus status);
char *mano_strdup(const char *text);
bool mano_size_add(size_t a, size_t b, size_t *out);
bool mano_size_mul(size_t a, size_t b, size_t *out);
ManoStatus mano_parse_double(const char *text, double *value);
void mano_error_print(const ManoError *error, FILE *stream);

#endif
