#include "common.h"

void mano_error_clear(ManoError *error) {
    if (!error) return;
    error->code = MANO_OK;
    error->line = error->column = error->row = 0;
    error->message[0] = '\0';
}

void mano_error_set(ManoError *error, ManoStatus code, size_t line,
                    size_t column, size_t row, const char *message) {
    if (!error) return;
    error->code = code;
    error->line = line;
    error->column = column;
    error->row = row;
    if (!message) message = "Error desconocido";
    (void)snprintf(error->message, sizeof(error->message), "%s", message);
}

const char *mano_status_name(ManoStatus status) {
    switch (status) {
        case MANO_OK: return "OK";
        case MANO_ERR_ARGUMENT: return "ARGUMENT";
        case MANO_ERR_MEMORY: return "MEMORY";
        case MANO_ERR_IO: return "IO";
        case MANO_ERR_PARSE: return "PARSE";
        case MANO_ERR_DATA: return "DATA";
        case MANO_ERR_TYPE: return "TYPE";
        case MANO_ERR_OVERFLOW: return "OVERFLOW";
        case MANO_ERR_UNSUPPORTED: return "UNSUPPORTED";
        case MANO_ERR_INTERNAL: return "INTERNAL";
        default: return "UNKNOWN";
    }
}

char *mano_strdup(const char *text) {
    if (!text) return NULL;
    size_t n = strlen(text);
    if (n == SIZE_MAX) return NULL;
    char *copy = (char *)malloc(n + 1);
    if (!copy) return NULL;
    memcpy(copy, text, n + 1);
    return copy;
}

bool mano_size_add(size_t a, size_t b, size_t *out) {
    if (!out || b > SIZE_MAX - a) return false;
    *out = a + b;
    return true;
}

bool mano_size_mul(size_t a, size_t b, size_t *out) {
    if (!out || (a != 0 && b > SIZE_MAX / a)) return false;
    *out = a * b;
    return true;
}

ManoStatus mano_parse_double(const char *text, double *value) {
    if (!text || !value) return MANO_ERR_ARGUMENT;
    while (*text == ' ' || *text == '\t') text++;
    if (*text == '\0') return MANO_ERR_TYPE;
    errno = 0;
    char *end = NULL;
    double parsed = strtod(text, &end);
    if (end == text || errno == ERANGE || !isfinite(parsed)) return MANO_ERR_TYPE;
    while (*end == ' ' || *end == '\t') end++;
    if (*end != '\0') return MANO_ERR_TYPE;
    *value = parsed;
    return MANO_OK;
}

void mano_error_print(const ManoError *error, FILE *stream) {
    if (!error || error->code == MANO_OK) return;
    if (!stream) stream = stderr;
    fprintf(stream, "Mano [%s]", mano_status_name(error->code));
    if (error->line) fprintf(stream, " línea %zu", error->line);
    if (error->row) fprintf(stream, " fila %zu", error->row);
    if (error->column) fprintf(stream, " columna %zu", error->column);
    fprintf(stream, ": %s\n", error->message);
}
