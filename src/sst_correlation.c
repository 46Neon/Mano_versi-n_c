#include "sst_correlation.h"

MilenaStatus sst_pearson(const double *x, const double *y, size_t count,
                       SstCorrelationResult *result, MilenaError *error) {
    if (!x || !y || !result) return MILENA_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));
    double mean_x = 0.0, mean_y = 0.0;
    double sum_xx = 0.0, sum_yy = 0.0, sum_xy = 0.0;
    for (size_t i = 0; i < count; i++) {
        if (!isfinite(x[i]) || !isfinite(y[i])) {
            result->invalid++;
            continue;
        }
        result->pairs++;
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        mean_x += dx / (double)result->pairs;
        mean_y += dy / (double)result->pairs;
        sum_xx += dx * (x[i] - mean_x);
        sum_yy += dy * (y[i] - mean_y);
        sum_xy += dx * (y[i] - mean_y);
    }
    if (result->pairs < 3 || sum_xx <= DBL_EPSILON || sum_yy <= DBL_EPSILON) {
        result->warning_small_sample = result->pairs < 3;
        milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0,
                       result->pairs < 3
                           ? "Pocos pares válidos para Pearson"
                           : "Pearson requiere variación en ambas variables");
        return MILENA_ERR_DATA;
    }
    result->coefficient = sum_xy / sqrt(sum_xx * sum_yy);
    result->valid = isfinite(result->coefficient);
    if (!result->valid) return MILENA_ERR_OVERFLOW;
    result->warning_small_sample = result->pairs < 30;
    return MILENA_OK;
}
