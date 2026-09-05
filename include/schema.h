#ifndef MANO_SCHEMA_H
#define MANO_SCHEMA_H

#include "common.h"

typedef enum {
    MANO_VAR_NUMERIC,
    MANO_VAR_CATEGORICAL,
    MANO_VAR_BINARY,
    MANO_VAR_TEXT
} ManoVariableType;

typedef enum {
    MANO_ROLE_FEATURE,
    MANO_ROLE_CATEGORICAL_INPUT,
    MANO_ROLE_BINARY_OUTPUT
} ManoVariableRole;

typedef struct {
    char *name;
    ManoVariableType type;
    ManoVariableRole role;
} ManoVariable;

typedef struct {
    ManoVariable *variables;
    size_t count;
    size_t capacity;
} ManoSchema;

void schema_init(ManoSchema *schema);
void schema_destroy(ManoSchema *schema);
ManoStatus schema_add(ManoSchema *schema, const char *name,
                      ManoVariableType type, ManoVariableRole role,
                      ManoError *error);
int schema_index(const ManoSchema *schema, const char *name);
const char *schema_type_name(ManoVariableType type);
const char *schema_role_name(ManoVariableRole role);

#endif
