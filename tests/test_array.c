#include "array.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expect_ok(MilenaStatus status, const MilenaError *error) {
    if (status != MILENA_OK) {
        fprintf(stderr, "array test failed: %s (%s)\n",
                error ? error->message : "sin detalle",
                milena_status_name(status));
        assert(status == MILENA_OK);
    }
}

static void test_creation_and_reshape(void) {
    const size_t shape[] = {2, 3};
    const double values[] = {1, 2, 3, 4, 5, 6};
    MilenaArray array = {0};
    MilenaArray view = {0};
    MilenaError error;
    milena_error_clear(&error);

    expect_ok(milena_array_from_f64(&array, 2, shape, values, &error), &error);
    assert(array.ndim == 2);
    assert(array.size == 6);
    assert(array.shape[0] == 2 && array.shape[1] == 3);
    assert(array.strides[0] == (ptrdiff_t)(3 * sizeof(double)));
    assert(array.strides[1] == (ptrdiff_t)sizeof(double));
    assert(milena_array_is_contiguous(&array));

    const size_t reshaped[] = {3, 2};
    expect_ok(milena_array_reshape_view(&view, &array, 2, reshaped, &error), &error);
    assert(!((view.flags & MILENA_ARRAY_OWN_DATA) != 0));
    assert(view.storage == array.storage);
    assert(((const double *)milena_array_const_data(&view))[4] == 5.0);

    milena_array_release(&view);
    milena_array_release(&array);
}

static void test_broadcast_add(void) {
    const size_t matrix_shape[] = {2, 3};
    const double matrix_values[] = {1, 2, 3, 4, 5, 6};
    const size_t vector_shape[] = {3};
    const double vector_values[] = {10, 20, 30};
    MilenaArray matrix = {0};
    MilenaArray vector = {0};
    MilenaArray result = {0};
    MilenaError error;
    milena_error_clear(&error);

    expect_ok(milena_array_from_f64(&matrix, 2, matrix_shape, matrix_values, &error), &error);
    expect_ok(milena_array_from_f64(&vector, 1, vector_shape, vector_values, &error), &error);
    expect_ok(milena_array_add(&result, &matrix, &vector, &error), &error);

    const double expected[] = {11, 22, 33, 14, 25, 36};
    assert(result.ndim == 2 && result.shape[0] == 2 && result.shape[1] == 3);
    assert(memcmp(milena_array_const_data(&result), expected, sizeof(expected)) == 0);

    milena_array_release(&result);
    milena_array_release(&vector);
    milena_array_release(&matrix);
}

static void test_sum_by_axis(void) {
    const size_t shape[] = {2, 3};
    const double values[] = {1, 2, 3, 4, 5, 6};
    MilenaArray array = {0};
    MilenaArray axis0 = {0};
    MilenaArray axis1 = {0};
    MilenaArray total = {0};
    MilenaError error;
    milena_error_clear(&error);

    expect_ok(milena_array_from_f64(&array, 2, shape, values, &error), &error);
    expect_ok(milena_array_sum(&axis0, &array, 0, false, &error), &error);
    expect_ok(milena_array_sum(&axis1, &array, 1, false, &error), &error);
    expect_ok(milena_array_sum(&total, &array, -1, false, &error), &error);

    const double expected_axis0[] = {5, 7, 9};
    const double expected_axis1[] = {6, 15};
    assert(axis0.ndim == 1 && axis0.shape[0] == 3);
    assert(axis1.ndim == 1 && axis1.shape[0] == 2);
    assert(memcmp(milena_array_const_data(&axis0), expected_axis0,
                  sizeof(expected_axis0)) == 0);
    assert(memcmp(milena_array_const_data(&axis1), expected_axis1,
                  sizeof(expected_axis1)) == 0);
    assert(total.ndim == 0);
    assert(((const double *)milena_array_const_data(&total))[0] == 21.0);

    milena_array_release(&total);
    milena_array_release(&axis1);
    milena_array_release(&axis0);
    milena_array_release(&array);
}

int main(void) {
    test_creation_and_reshape();
    test_broadcast_add();
    test_sum_by_axis();
    puts("OK: MilenaArray creation, views, broadcasting and reductions");
    return 0;
}
