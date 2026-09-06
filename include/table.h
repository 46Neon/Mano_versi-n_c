#ifndef MILENA_TABLE_H
#define MILENA_TABLE_H

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;
    MilenaArray values;
    bool *validity;
} MilenaTableColumn;

typedef struct {
    MilenaTableColumn *columns;
    size_t column_count;
    size_t row_count;
    size_t capacity;
} MilenaTable;

void milena_table_init(MilenaTable *table);
void milena_table_destroy(MilenaTable *table);

MilenaStatus milena_table_add_column_copy(MilenaTable *table,
                                          const char *name,
                                          const MilenaArray *values,
                                          const bool *validity,
                                          MilenaError *error);

int milena_table_column_index(const MilenaTable *table, const char *name);
const MilenaTableColumn *milena_table_column(const MilenaTable *table,
                                             size_t index);

MilenaStatus milena_table_filter(MilenaTable *out,
                                 const MilenaTable *source,
                                 const MilenaArray *mask,
                                 MilenaError *error);

#ifdef __cplusplus
}
#endif

#endif
