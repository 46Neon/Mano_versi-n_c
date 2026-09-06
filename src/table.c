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
        mask->ndim != 1 || mask->size != source->row_count) {
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
