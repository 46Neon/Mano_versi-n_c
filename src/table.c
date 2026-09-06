#include "table.h"

static void table_error(MilenaError *error, MilenaStatus code,
                        const char *message) {
    if (error) milena_error_set(error, code, 0, 0, 0, message);
}

void milena_table_init(MilenaTable *table) {
    if (!table) return;
    memset(table, 0, sizeof(*table));
}

void milena_table_destroy(MilenaTable *table) {
    if (!table) return;
    for (size_t i = 0; i < table->column_count; i++) {
        free(table->columns[i].name);
        free(table->columns[i].validity);
        milena_array_release(&table->columns[i].values);
    }
    free(table->columns);
    memset(table, 0, sizeof(*table));
}

static MilenaStatus table_reserve(MilenaTable *table, size_t required,
                                  MilenaError *error) {
    if (required <= table->capacity) return MILENA_OK;
    size_t capacity = table->capacity == 0 ? 4 : table->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            table_error(error, MILENA_ERR_OVERFLOW, "Demasiadas columnas en la tabla");
            return MILENA_ERR_OVERFLOW;
        }
        capacity *= 2;
    }
    MilenaTableColumn *columns = (MilenaTableColumn *)realloc(
        table->columns, capacity * sizeof(MilenaTableColumn));
    if (!columns) {
        table_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la tabla");
        return MILENA_ERR_MEMORY;
    }
    memset(columns + table->capacity, 0,
           (capacity - table->capacity) * sizeof(MilenaTableColumn));
    table->columns = columns;
    table->capacity = capacity;
    return MILENA_OK;
}

int milena_table_column_index(const MilenaTable *table, const char *name) {
    if (!table || !name) return -1;
    for (size_t i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) return (int)i;
    }
    return -1;
}

const MilenaTableColumn *milena_table_column(const MilenaTable *table,
                                             size_t index) {
    if (!table || index >= table->column_count) return NULL;
    return &table->columns[index];
}

MilenaStatus milena_table_add_column_copy(MilenaTable *table,
                                          const char *name,
                                          const MilenaArray *values,
                                          const bool *validity,
                                          MilenaError *error) {
    if (!table || !name || name[0] == '\0' || !values || !values->storage ||
        values->ndim != 1) {
        table_error(error, MILENA_ERR_ARGUMENT, "Una columna requiere nombre y array 1-D");
        return MILENA_ERR_ARGUMENT;
    }
    if (milena_table_column_index(table, name) >= 0) {
        table_error(error, MILENA_ERR_ARGUMENT, "La tabla ya contiene esa columna");
        return MILENA_ERR_ARGUMENT;
    }
    if (table->column_count > 0 && table->row_count != values->size) {
        table_error(error, MILENA_ERR_ARGUMENT, "Todas las columnas deben tener el mismo número de filas");
        return MILENA_ERR_ARGUMENT;
    }

    char *name_copy = milena_strdup(name);
    if (!name_copy) {
        table_error(error, MILENA_ERR_MEMORY, "No se pudo copiar el nombre de la columna");
        return MILENA_ERR_MEMORY;
    }
    MilenaArray values_copy = {0};
    size_t shape[] = {values->size};
    MilenaStatus status = milena_array_reshape_copy(&values_copy, values, 1,
                                                     shape, error);
    if (status != MILENA_OK) {
        free(name_copy);
        return status;
    }

    bool *validity_copy = NULL;
    if (values->size > 0) {
        validity_copy = (bool *)malloc(values->size * sizeof(bool));
        if (!validity_copy) {
            free(name_copy);
            milena_array_release(&values_copy);
            table_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la máscara de validez");
            return MILENA_ERR_MEMORY;
        }
        if (validity) memcpy(validity_copy, validity, values->size * sizeof(bool));
        else memset(validity_copy, true, values->size * sizeof(bool));
    }

    status = table_reserve(table, table->column_count + 1, error);
    if (status != MILENA_OK) {
        free(name_copy);
        free(validity_copy);
        milena_array_release(&values_copy);
        return status;
    }

    MilenaTableColumn *column = &table->columns[table->column_count];
    column->name = name_copy;
    column->values = values_copy;
    column->validity = validity_copy;
    table->column_count++;
    if (table->column_count == 1) table->row_count = values->size;
    return MILENA_OK;
}

MilenaStatus milena_table_filter(MilenaTable *out,
                                 const MilenaTable *source,
                                 const MilenaArray *mask,
                                 MilenaError *error) {
    if (!out || !source || !mask || out == source || mask->dtype != MILENA_DTYPE_BOOL ||
        mask->ndim != 1 || mask->size != source->row_count ||
        !milena_array_is_contiguous(mask)) {
        table_error(error, MILENA_ERR_ARGUMENT, "El filtro requiere una máscara bool con una entrada por fila");
        return MILENA_ERR_ARGUMENT;
    }
    milena_table_init(out);

    size_t selected = 0;
    const bool *mask_data = (const bool *)milena_array_const_data(mask);
    for (size_t i = 0; i < mask->size; i++) {
        if (mask_data[i]) selected++;
    }

    for (size_t column_index = 0; column_index < source->column_count; column_index++) {
        const MilenaTableColumn *column = &source->columns[column_index];
        MilenaArray selected_values = {0};
        MilenaStatus status = milena_array_boolean_mask(&selected_values,
                                                        &column->values,
                                                        mask, error);
        if (status != MILENA_OK) {
            milena_table_destroy(out);
            return status;
        }
        bool *selected_validity = NULL;
        if (selected > 0) {
            selected_validity = (bool *)malloc(selected * sizeof(bool));
            if (!selected_validity) {
                milena_array_release(&selected_values);
                milena_table_destroy(out);
                table_error(error, MILENA_ERR_MEMORY, "No se pudo reservar la validez filtrada");
                return MILENA_ERR_MEMORY;
            }
            size_t output_index = 0;
            for (size_t row = 0; row < source->row_count; row++) {
                if (mask_data[row]) {
                    selected_validity[output_index++] = column->validity ?
                        column->validity[row] : true;
                }
            }
        }
        status = milena_table_add_column_copy(out, column->name,
                                               &selected_values,
                                               selected_validity, error);
        free(selected_validity);
        milena_array_release(&selected_values);
        if (status != MILENA_OK) {
            milena_table_destroy(out);
            return status;
        }
    }
    return MILENA_OK;
}

MilenaStatus milena_table_select_columns(MilenaTable *out,
                                         const MilenaTable *source,
                                         const char *const *names,
                                         size_t name_count,
                                         MilenaError *error) {
    if (!out || !source || out == source || (name_count > 0 && !names)) {
        table_error(error, MILENA_ERR_ARGUMENT, "Argumentos inválidos para seleccionar columnas");
        return MILENA_ERR_ARGUMENT;
    }
    milena_table_init(out);
    for (size_t i = 0; i < name_count; i++) {
        int index = milena_table_column_index(source, names[i]);
        if (index < 0) {
            milena_table_destroy(out);
            table_error(error, MILENA_ERR_DATA, "La columna solicitada no existe");
            return MILENA_ERR_DATA;
        }
        const MilenaTableColumn *column = &source->columns[index];
        MilenaStatus status = milena_table_add_column_copy(out, column->name,
                                                            &column->values,
                                                            column->validity,
                                                            error);
        if (status != MILENA_OK) {
            milena_table_destroy(out);
            return status;
        }
    }
    return MILENA_OK;
}

MilenaStatus milena_table_fill_null_f64(MilenaTable *table,
                                        const char *column_name,
                                        double value,
                                        MilenaError *error) {
    if (!table || !column_name) {
        table_error(error, MILENA_ERR_ARGUMENT, "Argumentos inválidos para fill_null");
        return MILENA_ERR_ARGUMENT;
    }
    int index = milena_table_column_index(table, column_name);
    if (index < 0) {
        table_error(error, MILENA_ERR_DATA, "La columna de fill_null no existe");
        return MILENA_ERR_DATA;
    }
    MilenaTableColumn *column = &table->columns[index];
    if (column->values.dtype != MILENA_DTYPE_FLOAT64) {
        table_error(error, MILENA_ERR_TYPE, "fill_null_f64 requiere una columna float64");
        return MILENA_ERR_TYPE;
    }
    if (!column->validity) return MILENA_OK;
    double *data = (double *)milena_array_data(&column->values);
    for (size_t row = 0; row < table->row_count; row++) {
        if (!column->validity[row]) {
            data[row] = value;
            column->validity[row] = true;
        }
    }
    return MILENA_OK;
}

MilenaStatus milena_table_drop_null(MilenaTable *out,
                                    const MilenaTable *source,
                                    MilenaError *error) {
    if (!out || !source || out == source) {
        table_error(error, MILENA_ERR_ARGUMENT, "Argumentos inválidos para drop_null");
        return MILENA_ERR_ARGUMENT;
    }
    size_t shape[] = {source->row_count};
    MilenaArray mask = {0};
    MilenaStatus status = milena_array_zeros(&mask, MILENA_DTYPE_BOOL, 1,
                                             shape, error);
    if (status != MILENA_OK) return status;
    bool *mask_data = (bool *)milena_array_data(&mask);
    for (size_t row = 0; row < source->row_count; row++) {
        mask_data[row] = true;
        for (size_t column = 0; column < source->column_count; column++) {
            const bool *validity = source->columns[column].validity;
            if (validity && !validity[row]) {
                mask_data[row] = false;
                break;
            }
        }
    }
    status = milena_table_filter(out, source, &mask, error);
    milena_array_release(&mask);
    return status;
}

static int compare_sort_rows(const MilenaTableColumn *column,
                             size_t left, size_t right, bool ascending) {
    bool left_valid = !column->validity || column->validity[left];
    bool right_valid = !column->validity || column->validity[right];
    if (left_valid != right_valid) return left_valid ? -1 : 1;
    if (!left_valid) return 0;

    int comparison = 0;
    if (column->values.dtype == MILENA_DTYPE_INT64) {
        const int64_t *data = (const int64_t *)milena_array_const_data(&column->values);
        comparison = data[left] < data[right] ? -1 : (data[left] > data[right] ? 1 : 0);
    } else if (column->values.dtype == MILENA_DTYPE_FLOAT64) {
        const double *data = (const double *)milena_array_const_data(&column->values);
        comparison = data[left] < data[right] ? -1 : (data[left] > data[right] ? 1 : 0);
    } else {
        return 0;
    }
    return ascending ? comparison : -comparison;
}

static MilenaStatus table_copy_order(MilenaTable *out,
                                     const MilenaTable *source,
                                     const size_t *order,
                                     MilenaError *error) {
    milena_table_init(out);
    size_t shape[] = {source->row_count};
    for (size_t column_index = 0; column_index < source->column_count; column_index++) {
        const MilenaTableColumn *source_column = &source->columns[column_index];
        MilenaArray values = {0};
        MilenaStatus status = milena_array_zeros(&values, source_column->values.dtype,
                                                 1, shape, error);
        if (status != MILENA_OK) {
            milena_table_destroy(out);
            return status;
        }
        unsigned char *destination = (unsigned char *)milena_array_data(&values);
        const unsigned char *source_data =
            (const unsigned char *)milena_array_const_data(&source_column->values);
        bool *validity = source->row_count > 0 ?
            (bool *)malloc(source->row_count * sizeof(bool)) : NULL;
        if (source->row_count > 0 && !validity) {
            milena_array_release(&values);
            milena_table_destroy(out);
            table_error(error, MILENA_ERR_MEMORY, "No se pudo reservar validez para sort");
            return MILENA_ERR_MEMORY;
        }
        for (size_t row = 0; row < source->row_count; row++) {
            size_t source_row = order[row];
            memcpy(destination + row * values.itemsize,
                   source_data + source_row * values.itemsize,
                   values.itemsize);
            validity[row] = source_column->validity ?
                source_column->validity[source_row] : true;
        }
        status = milena_table_add_column_copy(out, source_column->name,
                                               &values, validity, error);
        free(validity);
        milena_array_release(&values);
        if (status != MILENA_OK) {
            milena_table_destroy(out);
            return status;
        }
    }
    return MILENA_OK;
}

MilenaStatus milena_table_sort(MilenaTable *out,
                               const MilenaTable *source,
                               const char *column_name,
                               bool ascending,
                               MilenaError *error) {
    if (!out || !source || !column_name || out == source) {
        table_error(error, MILENA_ERR_ARGUMENT, "Argumentos inválidos para sort");
        return MILENA_ERR_ARGUMENT;
    }
    int index = milena_table_column_index(source, column_name);
    if (index < 0) {
        table_error(error, MILENA_ERR_DATA, "La columna de sort no existe");
        return MILENA_ERR_DATA;
    }
    const MilenaTableColumn *column = &source->columns[index];
    if (column->values.dtype != MILENA_DTYPE_INT64 &&
        column->values.dtype != MILENA_DTYPE_FLOAT64) {
        table_error(error, MILENA_ERR_UNSUPPORTED, "sort admite int64 y float64 en esta etapa");
        return MILENA_ERR_UNSUPPORTED;
    }
    size_t *order = source->row_count > 0 ?
        (size_t *)malloc(source->row_count * sizeof(size_t)) : NULL;
    if (source->row_count > 0 && !order) {
        table_error(error, MILENA_ERR_MEMORY, "No se pudo reservar el orden de sort");
        return MILENA_ERR_MEMORY;
    }
    for (size_t i = 0; i < source->row_count; i++) order[i] = i;
    for (size_t i = 1; i < source->row_count; i++) {
        size_t current = order[i];
        size_t position = i;
        while (position > 0 && compare_sort_rows(column, order[position - 1],
                                                  current, ascending) > 0) {
            order[position] = order[position - 1];
            position--;
        }
        order[position] = current;
    }
    MilenaStatus status = table_copy_order(out, source, order, error);
    free(order);
    return status;
}
