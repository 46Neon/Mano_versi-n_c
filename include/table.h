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

typedef enum {
    MILENA_AGG_COUNT = 0,
    MILENA_AGG_SUM,
    MILENA_AGG_MEAN,
    MILENA_AGG_MIN,
    MILENA_AGG_MAX
} MilenaAggregateOp;

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
MilenaStatus milena_table_select_columns(MilenaTable *out,
                                         const MilenaTable *source,
                                         const char *const *names,
                                         size_t name_count,
                                         MilenaError *error);
MilenaStatus milena_table_fill_null_f64(MilenaTable *table,
                                        const char *column_name,
                                        double value,
                                        MilenaError *error);
MilenaStatus milena_table_drop_null(MilenaTable *out,
                                    const MilenaTable *source,
                                    MilenaError *error);
MilenaStatus milena_table_sort(MilenaTable *out,
                               const MilenaTable *source,
                               const char *column_name,
                               bool ascending,
                               MilenaError *error);
MilenaStatus milena_table_group_by_aggregate(MilenaTable *out,
                                             const MilenaTable *source,
                                             const char *key_column,
                                             const char *value_column,
                                             MilenaAggregateOp operation,
                                             MilenaError *error);

#ifdef __cplusplus
}
#endif

#endif
