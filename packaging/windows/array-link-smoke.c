#include "array.h"
#include <stdio.h>

int main(void) {
    size_t shape[] = {3};
    int64_t values[] = {1, 2, 3};
    MilenaArray array = {0};
    MilenaError error;
    milena_error_clear(&error);
    if (milena_array_from_i64(&array, 1, shape, values, &error) != MILENA_OK) {
        milena_error_print(&error, stderr);
        return 1;
    }
    if (array.size != 3 || array.shape[0] != 3 || array.dtype != MILENA_DTYPE_INT64) {
        milena_array_release(&array);
        return 2;
    }
    printf("array-link-smoke: dtype=%s size=%zu\n",
           milena_dtype_name(array.dtype), array.size);
    milena_array_release(&array);
    return 0;
}
