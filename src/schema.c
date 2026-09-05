#include "schema.h"

void schema_init(ManoSchema *schema) {
    if (!schema) return;
    memset(schema, 0, sizeof(*schema));
}

void schema_destroy(ManoSchema *schema) {
    if (!schema) return;
    for (size_t i = 0; i < schema->count; i++) free(schema->variables[i].name);
    free(schema->variables);
    schema_init(schema);
}

static ManoStatus schema_grow(ManoSchema *schema) {
    if (schema->count < schema->capacity) return MANO_OK;
    size_t next = schema->capacity ? schema->capacity * 2 : 8;
    if (next < schema->capacity || next > SIZE_MAX / sizeof(*schema->variables)) {
        return MANO_ERR_OVERFLOW;
    }
    ManoVariable *tmp = (ManoVariable *)realloc(schema->variables,
                                                 next * sizeof(*tmp));
    if (!tmp) return MANO_ERR_MEMORY;
    schema->variables = tmp;
    schema->capacity = next;
    return MANO_OK;
}

int schema_index(const ManoSchema *schema, const char *name) {
    if (!schema || !name) return -1;
    for (size_t i = 0; i < schema->count; i++) {
        if (strcmp(schema->variables[i].name, name) == 0) return (int)i;
    }
    return -1;
}

ManoStatus schema_add(ManoSchema *schema, const char *name,
                      ManoVariableType type, ManoVariableRole role,
                      ManoError *error) {
    if (!schema || !name || name[0] == '\0') return MANO_ERR_ARGUMENT;
    int existing = schema_index(schema, name);
    if (existing >= 0) {
        schema->variables[existing].type = type;
        schema->variables[existing].role = role;
        return MANO_OK;
    }
    ManoStatus status = schema_grow(schema);
    if (status != MANO_OK) {
        mano_error_set(error, status, 0, 0, 0, "Sin memoria para esquema");
        return status;
    }
    char *copy = mano_strdup(name);
    if (!copy) {
        mano_error_set(error, MANO_ERR_MEMORY, 0, 0, 0,
                       "Sin memoria para nombre de variable");
        return MANO_ERR_MEMORY;
    }
    schema->variables[schema->count].name = copy;
    schema->variables[schema->count].type = type;
    schema->variables[schema->count].role = role;
    schema->count++;
    return MANO_OK;
}

const char *schema_type_name(ManoVariableType type) {
    switch (type) {
        case MANO_VAR_NUMERIC: return "numerica";
        case MANO_VAR_CATEGORICAL: return "categorica";
        case MANO_VAR_BINARY: return "binaria";
        case MANO_VAR_TEXT: return "texto";
        default: return "desconocida";
    }
}

const char *schema_role_name(ManoVariableRole role) {
    switch (role) {
        case MANO_ROLE_FEATURE: return "variable";
        case MANO_ROLE_CATEGORICAL_INPUT: return "entrada_categorica";
        case MANO_ROLE_BINARY_OUTPUT: return "salida_binaria";
        default: return "desconocido";
    }
}
