#include "sst_advanced.h"

typedef struct {
    double value;
    double weight;
} WeightedValue;

static int compare_weighted_values(const void *left, const void *right) {
    const WeightedValue *a = (const WeightedValue *)left;
    const WeightedValue *b = (const WeightedValue *)right;
    return a->value < b->value ? -1 : (a->value > b->value ? 1 : 0);
}

static double weighted_percentile(const WeightedValue *items, size_t count,
                                  double total_weight, double percentile) {
    if (!items || count == 0 || total_weight <= 0.0) return 0.0;
    double target = total_weight * percentile;
    double cumulative = 0.0;
    for (size_t i = 0; i < count; i++) {
        cumulative += items[i].weight;
        if (cumulative >= target) return items[i].value;
    }
    return items[count - 1].value;
}

ManoStatus sst_advanced_compute(const double *values, const double *weights,
                                size_t count, SstAdvancedStats *result,
                                ManoError *error) {
    if (!values || !result) return MANO_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));
    if (count == 0) {
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0,
                       "No hay observaciones para estadísticas avanzadas");
        return MANO_ERR_DATA;
    }
    WeightedValue *items = (WeightedValue *)calloc(count, sizeof(*items));
    if (!items) return MANO_ERR_MEMORY;
    double weight_total = 0.0;
    for (size_t i = 0; i < count; i++) {
        double weight = weights ? weights[i] : 1.0;
        if (!isfinite(values[i]) || !isfinite(weight) || weight <= 0.0) {
            result->invalid++;
            continue;
        }
        items[result->count].value = values[i];
        items[result->count].weight = weight;
        result->count++;
        weight_total += weight;
    }
    if (result->count == 0 || weight_total <= 0.0) {
        free(items);
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0,
                       "No existen valores válidos ponderables");
        return MANO_ERR_DATA;
    }
    qsort(items, result->count, sizeof(*items), compare_weighted_values);
    result->total_weight = weight_total;
    for (size_t i = 0; i < result->count; i++) {
        result->mean += items[i].value * items[i].weight;
    }
    result->mean /= weight_total;
    double m2 = 0.0, m3 = 0.0, m4 = 0.0;
    for (size_t i = 0; i < result->count; i++) {
        double delta = items[i].value - result->mean;
        double delta2 = delta * delta;
        m2 += items[i].weight * delta2;
        m3 += items[i].weight * delta2 * delta;
        m4 += items[i].weight * delta2 * delta2;
    }
    result->variance = m2 / weight_total;
    result->standard_deviation = result->variance > 0.0 ? sqrt(result->variance) : 0.0;
    if (fabs(result->mean) > DBL_EPSILON) {
        result->coefficient_variation = result->standard_deviation / fabs(result->mean);
    }
    if (result->variance > DBL_EPSILON) {
        result->skewness = (m3 / weight_total) /
                           pow(result->variance, 1.5);
        result->excess_kurtosis = (m4 / weight_total) /
                                  (result->variance * result->variance) - 3.0;
    }
    result->p90 = weighted_percentile(items, result->count, weight_total, 0.90);
    result->p95 = weighted_percentile(items, result->count, weight_total, 0.95);
    result->valid = true;
    free(items);
    return MANO_OK;
}
