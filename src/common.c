#include "common.h"

void mano_error_init(ManoErrorInfo *error) {
    if (error) {
        error->code = MANO_OK;
        error->message[0] = '\0';
        error->line = 0;
        error->column = 0;
    }
}

void mano_error_set(ManoErrorInfo *error, ManoError code, const char *msg, int line, int column) {
    if (error) {
        error->code = code;
        snprintf(error->message, sizeof(error->message), "%s", msg ? msg : "Error desconocido");
        error->line = line;
        error->column = column;
    }
}

void mano_error_print(const ManoErrorInfo *error) {
    if (error && error->code != MANO_OK) {
        fprintf(stderr, "Error [%d] en línea %d:%d - %s\n", 
                error->code, error->line, error->column, error->message);
    }
}
