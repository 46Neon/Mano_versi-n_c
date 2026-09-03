#ifndef MANO_DATASET_H
#define MANO_DATASET_H

#include "common.h"

typedef struct Dataset {
    char *filename;
    char **headers;
    char ***rows;
    size_t column_count;
    size_t row_count;
    size_t row_capacity;
    size_t invalid_rows;
} Dataset;

bool dataset_cargar_csv(Dataset *dataset, const char *filename);
bool dataset_cargar_json(Dataset *dataset, const char *filename);
bool dataset_guardar_json(const Dataset *dataset, const char *filename);
void dataset_destruir(Dataset *dataset);
void dataset_imprimir(const Dataset *dataset, size_t max_rows);
int dataset_indice_columna(const Dataset *dataset, const char *name);
bool dataset_clean_nulls(Dataset *dataset, const char *strategy);
bool dataset_clean_duplicates(Dataset *dataset, const char *strategy);
bool dataset_filter_condition(Dataset *dataset, const char *condition);
bool dataset_transform_total(Dataset *dataset, const char *expression);
bool dataset_transform_period(Dataset *dataset, const char *column);
bool dataset_group_by(Dataset *dataset, const char *column);
bool dataset_aggregate_sum(Dataset *dataset, const char *column);
bool dataset_aggregate_avg(Dataset *dataset, const char *column);
bool dataset_aggregate_min(Dataset *dataset, const char *column);
bool dataset_aggregate_max(Dataset *dataset, const char *column);

#endif
