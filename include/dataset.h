#ifndef MANO_DATASET_H
#define MANO_DATASET_H

#include "common.h"

typedef struct {
    size_t max_rows;
    size_t max_columns;
    size_t max_field_bytes;
} DatasetLimits;

typedef struct {
    char *filename;
    char **headers;
    char ***rows;
    size_t column_count;
    size_t row_count;
    size_t row_capacity;
    size_t invalid_rows;
} Dataset;

DatasetLimits dataset_default_limits(void);
void dataset_init(Dataset *dataset);
void dataset_destroy(Dataset *dataset);
ManoStatus dataset_load_csv_with_limits(Dataset *dataset, const char *filename,
                                        char delimiter, const DatasetLimits *limits,
                                        ManoError *error);
ManoStatus dataset_load_csv(Dataset *dataset, const char *filename,
                            char delimiter, ManoError *error);
ManoStatus dataset_save_json(const Dataset *dataset, const char *filename,
                             ManoError *error);
int dataset_column_index(const Dataset *dataset, const char *name);
ManoStatus dataset_remove_null_rows(Dataset *dataset, ManoError *error);
ManoStatus dataset_remove_duplicates(Dataset *dataset, ManoError *error);
ManoStatus dataset_add_product(Dataset *dataset, const char *left,
                               const char *right, const char *output,
                               ManoError *error);
ManoStatus dataset_add_month(Dataset *dataset, const char *date_column,
                             const char *output, ManoError *error);
ManoStatus dataset_filter_positive_product(Dataset *dataset,
                                           const char *left,
                                           const char *right,
                                           ManoError *error);
void dataset_print(const Dataset *dataset, size_t max_rows, FILE *stream);

/* Compatibility names retained from the original project. */
bool dataset_cargar_csv(Dataset *dataset, const char *filename);
bool dataset_guardar_json(const Dataset *dataset, const char *filename);
void dataset_destruir(Dataset *dataset);
int dataset_indice_columna(const Dataset *dataset, const char *name);

#endif
