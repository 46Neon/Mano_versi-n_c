#include "schema.h"

void schema_init(MilenaSchema *schema) {
    if (!schema) return;
    memset(schema, 0, sizeof(*schema));
}

void schema_destroy(MilenaSchema *schema) {
    if (!schema) return;
    for (size_t i = 0; i < schema->count; i++) free(schema->variables[i].name);
    free(schema->variables);
    schema_init(schema);
}

static MilenaStatus schema_grow(MilenaSchema *schema) {
    if (schema->count < schema->capacity) return MILENA_OK;
    size_t next = schema->capacity ? schema->capacity * 2 : 8;
    if (next < schema->capacity || next > SIZE_MAX / sizeof(*schema->variables)) {
        return MILENA_ERR_OVERFLOW;
    }
    MilenaVariable *tmp = (MilenaVariable *)realloc(schema->variables,
                                                 next * sizeof(*tmp));
    if (!tmp) return MILENA_ERR_MEMORY;
    schema->variables = tmp;
    schema->capacity = next;
    return MILENA_OK;
}

int schema_index(const MilenaSchema *schema, const char *name) {
    if (!schema || !name) return -1;
    for (size_t i = 0; i < schema->count; i++) {
        if (strcmp(schema->variables[i].name, name) == 0) return (int)i;
    }
    return -1;
}

MilenaStatus schema_add(MilenaSchema *schema, const char *name,
                      MilenaVariableType type, MilenaVariableRole role,
                      MilenaError *error) {
    if (!schema || !name || name[0] == '\0') return MILENA_ERR_ARGUMENT;
    int existing = schema_index(schema, name);
    if (existing >= 0) {
        schema->variables[existing].type = type;
        schema->variables[existing].role = role;
        return MILENA_OK;
    }
    MilenaStatus status = schema_grow(schema);
    if (status != MILENA_OK) {
        milena_error_set(error, status, 0, 0, 0, "Sin memoria para esquema");
        return status;
    }
    char *copy = milena_strdup(name);
    if (!copy) {
        milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, 0,
                       "Sin memoria para nombre de variable");
        return MILENA_ERR_MEMORY;
    }
    schema->variables[schema->count].name = copy;
    schema->variables[schema->count].type = type;
    schema->variables[schema->count].role = role;
    schema->count++;
    return MILENA_OK;
}

const char *schema_type_name(MilenaVariableType type) {
    switch (type) {
        case MILENA_VAR_NUMERIC: return "numerica";
        case MILENA_VAR_CATEGORICAL: return "categorica";
        case MILENA_VAR_BINARY: return "binaria";
        case MILENA_VAR_TEXT: return "texto";
        default: return "desconocida";
    }
}

const char *schema_role_name(MilenaVariableRole role) {
    switch (role) {
        case MILENA_ROLE_FEATURE: return "variable";
        case MILENA_ROLE_CATEGORICAL_INPUT: return "entrada_categorica";
        case MILENA_ROLE_BINARY_OUTPUT: return "salida_binaria";
        default: return "desconocido";
    }
}
