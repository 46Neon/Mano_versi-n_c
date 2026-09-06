#include "table.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expect_ok(MilenaStatus status, const MilenaError *error) {
    if (status != MILENA_OK) {
        fprintf(stderr, "table test failed: %s (%s)\n",
                error ? error->message : "sin detalle",
                milena_status_name(status));
        assert(status == MILENA_OK);
    }
}

int main(void) {
    const size_t shape[] = {4};
    const int64_t ids[] = {10, 11, 12, 13};
    const double amounts[] = {100.0, 200.0, 300.0, 400.0};
    const bool valid_amounts[] = {true, false, true, true};
    const bool filter_values[] = {true, false, true, false};
    MilenaError error;
    milena_error_clear(&error);

    MilenaArray id_array = {0};
    MilenaArray amount_array = {0};
    MilenaArray filter = {0};
    expect_ok(milena_array_from_i64(&id_array, 1, shape, ids, &error), &error);
    expect_ok(milena_array_from_f64(&amount_array, 1, shape, amounts, &error), &error);
    expect_ok(milena_array_zeros(&filter, MILENA_DTYPE_BOOL, 1, shape, &error), &error);
    memcpy(milena_array_data(&filter), filter_values, sizeof(filter_values));

    MilenaTable table;
    milena_table_init(&table);
    expect_ok(milena_table_add_column_copy(&table, "id", &id_array, NULL, &error), &error);
    expect_ok(milena_table_add_column_copy(&table, "amount", &amount_array,
                                           valid_amounts, &error), &error);
    assert(table.column_count == 2);
    assert(table.row_count == 4);
    assert(milena_table_column_index(&table, "amount") == 1);
    assert(milena_table_column_index(&table, "missing") == -1);

    MilenaTable filtered;
    expect_ok(milena_table_filter(&filtered, &table, &filter, &error), &error);
    assert(filtered.column_count == 2);
    assert(filtered.row_count == 2);
    const MilenaTableColumn *filtered_ids = milena_table_column(&filtered, 0);
    const MilenaTableColumn *filtered_amounts = milena_table_column(&filtered, 1);
    assert(filtered_ids && filtered_amounts);
    assert(((const int64_t *)milena_array_const_data(&filtered_ids->values))[0] == 10);
    assert(((const int64_t *)milena_array_const_data(&filtered_ids->values))[1] == 12);
    assert(((const double *)milena_array_const_data(&filtered_amounts->values))[0] == 100.0);
    assert(((const double *)milena_array_const_data(&filtered_amounts->values))[1] == 300.0);
    assert(filtered_amounts->validity[0] == true);
    assert(filtered_amounts->validity[1] == true);

    const char *selected_names[] = {"amount"};
    MilenaTable selected_columns;
    expect_ok(milena_table_select_columns(&selected_columns, &table,
                                          selected_names, 1, &error), &error);
    assert(selected_columns.column_count == 1);
    assert(strcmp(selected_columns.columns[0].name, "amount") == 0);

    MilenaTable dropped;
    expect_ok(milena_table_drop_null(&dropped, &table, &error), &error);
    assert(dropped.row_count == 3);

    MilenaTable sorted;
    expect_ok(milena_table_sort(&sorted, &table, "amount", true, &error), &error);
    const MilenaTableColumn *sorted_amounts = milena_table_column(&sorted, 1);
    assert(sorted_amounts);
    assert(((const double *)milena_array_const_data(&sorted_amounts->values))[0] == 100.0);
    assert(((const double *)milena_array_const_data(&sorted_amounts->values))[1] == 300.0);
    assert(((const double *)milena_array_const_data(&sorted_amounts->values))[2] == 400.0);
    assert(sorted_amounts->validity[3] == false);

    expect_ok(milena_table_fill_null_f64(&table, "amount", 0.0, &error), &error);
    const MilenaTableColumn *filled_amounts = milena_table_column(&table, 1);
    assert(filled_amounts);
    assert(((const double *)milena_array_const_data(&filled_amounts->values))[1] == 0.0);
    assert(filled_amounts->validity[1] == true);

    milena_table_destroy(&sorted);
    milena_table_destroy(&dropped);
    milena_table_destroy(&selected_columns);
    milena_table_destroy(&filtered);
    milena_table_destroy(&table);
    milena_array_release(&filter);
    milena_array_release(&amount_array);
    milena_array_release(&id_array);
    puts("OK: MilenaTable columns, validity and filtering");
    return 0;
}
