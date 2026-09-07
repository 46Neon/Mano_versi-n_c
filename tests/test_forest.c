#include "forest.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const size_t shape[] = {4, 2};
    const double values[] = {0, 0, 0, 1, 1, 0, 1, 1};
    const int64_t labels[] = {0, 0, 1, 1};
    MilenaArray features = {0}, target = {0}, predicted = {0};
    MilenaError error; milena_error_clear(&error);
    assert(milena_array_from_f64(&features, 2, shape, values, &error) == MILENA_OK);
    const size_t label_shape[] = {4};
    assert(milena_array_from_i64(&target, 1, label_shape, labels, &error) == MILENA_OK);
    MilenaForestClassifier forest; milena_forest_init(&forest);
    assert(milena_forest_train(&forest, &features, &target, 3, &error) == MILENA_OK);
    assert(milena_forest_predict(&forest, &features, &predicted, &error) == MILENA_OK);
    const int64_t *result = milena_array_const_data(&predicted);
    for (size_t i = 0; i < 4; i++) assert(result[i] == labels[i]);
    milena_array_release(&predicted); milena_forest_release(&forest);
    milena_array_release(&target); milena_array_release(&features);
    puts("OK: Milena forest classifier"); return 0;
}
