#include "sst_contingency.h"

static void free_labels(char **labels, size_t count) {
    if (!labels) return;
    for (size_t i = 0; i < count; i++) free(labels[i]);
    free(labels);
}

void sst_contingency_init(SstContingency2D *table) {
    if (!table) return;
    memset(table, 0, sizeof(*table));
}

void sst_contingency_destroy(SstContingency2D *table) {
    if (!table) return;
    free_labels(table->row_labels, table->row_count);
    free_labels(table->column_labels, table->column_count);
    free(table->cells);
    sst_contingency_init(table);
}

static int find_label(char *const *labels, size_t count, const char *value) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(labels[i], value ? value : "") == 0) return (int)i;
    }
    return -1;
}

static MilenaStatus add_label(char ***labels, size_t *count, size_t *capacity,
                            const char *value) {
    if (*count == *capacity) {
        size_t next = *capacity ? *capacity * 2 : 8;
        char **tmp = (char **)realloc(*labels, next * sizeof(*tmp));
        if (!tmp) return MILENA_ERR_MEMORY;
        *labels = tmp;
        *capacity = next;
    }
    (*labels)[*count] = milena_strdup(value ? value : "");
    if (!(*labels)[*count]) return MILENA_ERR_MEMORY;
    (*count)++;
    return MILENA_OK;
}

MilenaStatus sst_contingency_build(const char *const *rows,
                                 const char *const *columns,
                                 size_t count,
                                 SstContingency2D *table,
                                 MilenaError *error) {
    if (!rows || !columns || !table) return MILENA_ERR_ARGUMENT;
    sst_contingency_destroy(table);
    size_t row_capacity = 0, column_capacity = 0;
    for (size_t i = 0; i < count; i++) {
        if (find_label(table->row_labels, table->row_count, rows[i] ? rows[i] : "") < 0) {
            MilenaStatus status = add_label(&table->row_labels, &table->row_count,
                                          &row_capacity, rows[i]);
            if (status != MILENA_OK) goto fail;
        }
        if (find_label(table->column_labels, table->column_count,
                       columns[i] ? columns[i] : "") < 0) {
            MilenaStatus status = add_label(&table->column_labels,
                                          &table->column_count,
                                          &column_capacity, columns[i]);
            if (status != MILENA_OK) goto fail;
        }
    }
    if (!table->row_count || !table->column_count) goto fail_data;
    if (table->row_count > SIZE_MAX / table->column_count) goto fail_overflow;
    table->cells = (size_t *)calloc(table->row_count * table->column_count,
                                    sizeof(*table->cells));
    if (!table->cells) goto fail_memory;
    for (size_t i = 0; i < count; i++) {
        int r = find_label(table->row_labels, table->row_count, rows[i]);
        int c = find_label(table->column_labels, table->column_count, columns[i]);
        if (r >= 0 && c >= 0) {
            table->cells[(size_t)r * table->column_count + (size_t)c]++;
        }
    }
    return MILENA_OK;

fail_memory:
    milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, 0, "Sin memoria para tabla de contingencia");
    sst_contingency_destroy(table);
    return MILENA_ERR_MEMORY;
fail_overflow:
    milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0, "Tabla de contingencia demasiado grande");
    sst_contingency_destroy(table);
    return MILENA_ERR_OVERFLOW;
fail_data:
    milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0, "Tabla de contingencia vacía");
    sst_contingency_destroy(table);
    return MILENA_ERR_DATA;
fail:
    milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, 0, "Sin memoria para categorías");
    sst_contingency_destroy(table);
    return MILENA_ERR_MEMORY;
}

MilenaStatus sst_contingency_chi_square(const SstContingency2D *table,
                                      SstChiSquareResult *result,
                                      MilenaError *error) {
    if (!table || !result || !table->cells || table->row_count < 2 ||
        table->column_count < 2) return MILENA_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));
    size_t total = 0;
    for (size_t r = 0; r < table->row_count; r++) {
        for (size_t c = 0; c < table->column_count; c++) {
            total += table->cells[r * table->column_count + c];
        }
    }
    if (total == 0) {
        milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0, "Contingencia sin observaciones");
        return MILENA_ERR_DATA;
    }
    double *row_totals = (double *)calloc(table->row_count, sizeof(*row_totals));
    double *column_totals = (double *)calloc(table->column_count, sizeof(*column_totals));
    if (!row_totals || !column_totals) {
        free(row_totals); free(column_totals);
        return MILENA_ERR_MEMORY;
    }
    for (size_t r = 0; r < table->row_count; r++) {
        for (size_t c = 0; c < table->column_count; c++) {
            size_t value = table->cells[r * table->column_count + c];
            row_totals[r] += (double)value;
            column_totals[c] += (double)value;
        }
    }
    for (size_t r = 0; r < table->row_count; r++) {
        for (size_t c = 0; c < table->column_count; c++) {
            double expected = row_totals[r] * column_totals[c] / (double)total;
            if (expected < 5.0) result->low_expected_cells++;
            if (expected > 0.0) {
                double observed = (double)table->cells[r * table->column_count + c];
                double delta = observed - expected;
                result->statistic += delta * delta / expected;
            }
        }
    }
    result->degrees_of_freedom = (table->row_count - 1) * (table->column_count - 1);
    result->valid = true;
    free(row_totals);
    free(column_totals);
    return MILENA_OK;
}
