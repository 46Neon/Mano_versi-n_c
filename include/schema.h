#ifndef MILENA_SCHEMA_H
#define MILENA_SCHEMA_H

#include "common.h"

typedef enum {
    MILENA_VAR_NUMERIC,
    MILENA_VAR_CATEGORICAL,
    MILENA_VAR_BINARY,
    MILENA_VAR_TEXT
} MilenaVariableType;

typedef enum {
    MILENA_ROLE_FEATURE,
    MILENA_ROLE_CATEGORICAL_INPUT,
    MILENA_ROLE_BINARY_OUTPUT
} MilenaVariableRole;

typedef struct {
    char *name;
    MilenaVariableType type;
    MilenaVariableRole role;
} MilenaVariable;

typedef struct {
    MilenaVariable *variables;
    size_t count;
    size_t capacity;
} MilenaSchema;

void schema_init(MilenaSchema *schema);
void schema_destroy(MilenaSchema *schema);
MilenaStatus schema_add(MilenaSchema *schema, const char *name,
                      MilenaVariableType type, MilenaVariableRole role,
                      MilenaError *error);
int schema_index(const MilenaSchema *schema, const char *name);
const char *schema_type_name(MilenaVariableType type);
const char *schema_role_name(MilenaVariableRole role);

#endif
