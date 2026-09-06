#include "sst_inference.h"

typedef struct {
    double value;
    int group;
} RankedValue;

static double normal_cdf(double value) {
    return 0.5 * erfc(-value / sqrt(2.0));
}

static double two_sided_normal_p(double z) {
    return 2.0 * (1.0 - normal_cdf(fabs(z)));
}

static double poisson_cdf(size_t k, double lambda) {
    if (lambda == 0.0) return 1.0;
    if (lambda > 700.0) return 0.0;
    long double term = expl(-(long double)lambda);
    long double sum = term;
    for (size_t i = 1; i <= k; i++) {
        term *= (long double)lambda / (long double)i;
        sum += term;
        if (term < 1e-18L * sum) break;
    }
    if (sum < 0.0L) return 0.0;
    if (sum > 1.0L) return 1.0;
    return (double)sum;
}

static double solve_poisson_lower(size_t events, double target) {
    double low = 0.0, high = events > 0 ? (double)events : 1.0;
    for (size_t i = 0; i < 100; i++) {
        double middle = (low + high) / 2.0;
        double tail = events == 0 ? 1.0 : 1.0 - poisson_cdf(events - 1, middle);
        if (tail < target) low = middle;
        else high = middle;
    }
    return (low + high) / 2.0;
}

static double solve_poisson_upper(size_t events, double target) {
    double low = 0.0, high = events > 0 ? (double)events + 1.0 : 1.0;
    while (poisson_cdf(events, high) > target && high < 1e6) high *= 2.0;
    for (size_t i = 0; i < 100; i++) {
        double middle = (low + high) / 2.0;
        if (poisson_cdf(events, middle) > target) low = middle;
        else high = middle;
    }
    return (low + high) / 2.0;
}

static int compare_double(const void *left, const void *right) {
    const double a = *(const double *)left;
    const double b = *(const double *)right;
    return a < b ? -1 : (a > b ? 1 : 0);
}

MilenaStatus sst_poisson_rate_interval(size_t events, double exposure,
                                     double factor, double z_value,
                                     SstPoissonInterval *result,
                                     MilenaError *error) {
    if (!result || !isfinite(exposure) || !isfinite(factor) ||
        !isfinite(z_value) || exposure <= 0.0 || factor <= 0.0 || z_value <= 0.0) {
        return MILENA_ERR_ARGUMENT;
    }
    memset(result, 0, sizeof(*result));
    result->events = events;
    result->exposure = exposure;
    result->factor = factor;
    result->rate = (double)events / exposure * factor;
    double standard_error = sqrt((double)events) / exposure * factor;
    result->lower = result->rate - z_value * standard_error;
    result->upper = result->rate + z_value * standard_error;
    if (result->lower < 0.0) result->lower = 0.0;
    if (events == 0) result->upper = z_value / exposure * factor;
    result->approximate = true;
    if (!isfinite(result->lower) || !isfinite(result->upper)) {
        milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0,
                       "Intervalo Poisson no finito");
        return MILENA_ERR_OVERFLOW;
    }
    return MILENA_OK;
}

MilenaStatus sst_poisson_exact_interval(size_t events, double exposure,
                                      double factor, double confidence_level,
                                      SstPoissonInterval *result,
                                      MilenaError *error) {
    if (!result || !isfinite(exposure) || !isfinite(factor) ||
        !isfinite(confidence_level) || exposure <= 0.0 || factor <= 0.0 ||
        confidence_level <= 0.0 || confidence_level >= 1.0) {
        return MILENA_ERR_ARGUMENT;
    }
    memset(result, 0, sizeof(*result));
    double alpha = 1.0 - confidence_level;
    double lower_lambda = events == 0 ? 0.0 : solve_poisson_lower(events, alpha / 2.0);
    double upper_lambda = solve_poisson_upper(events, alpha / 2.0);
    result->events = events;
    result->exposure = exposure;
    result->factor = factor;
    result->rate = (double)events / exposure * factor;
    result->lower = lower_lambda / exposure * factor;
    result->upper = upper_lambda / exposure * factor;
    result->approximate = upper_lambda > 700.0;
    if (!isfinite(result->lower) || !isfinite(result->upper)) {
        milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0,
                       "Intervalo Poisson no finito");
        return MILENA_ERR_OVERFLOW;
    }
    return MILENA_OK;
}

MilenaStatus sst_risk_ratio_odds_ratio(size_t exposed_events,
                                      size_t exposed_non_events,
                                      size_t control_events,
                                      size_t control_non_events,
                                      SstRiskMeasure *result,
                                      MilenaError *error) {
    if (!result) return MILENA_ERR_ARGUMENT;
    memset(result, 0, sizeof(*result));
    double a = (double)exposed_events;
    double b = (double)exposed_non_events;
    double c = (double)control_events;
    double d = (double)control_non_events;
    if (a + b <= 0.0 || c + d <= 0.0) {
        milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0,
                       "Grupos insuficientes para riesgo relativo");
        return MILENA_ERR_DATA;
    }
    if (a == 0.0 || b == 0.0 || c == 0.0 || d == 0.0) {
        a += 0.5; b += 0.5; c += 0.5; d += 0.5;
        result->continuity_correction = true;
    }
    result->exposed_events = exposed_events;
    result->exposed_non_events = exposed_non_events;
    result->control_events = control_events;
    result->control_non_events = control_non_events;
    result->risk_exposed = a / (a + b);
    result->risk_control = c / (c + d);
    result->relative_risk = result->risk_exposed / result->risk_control;
    result->odds_ratio = (a * d) / (b * c);
    double z = 1.959963984540054;
    double se_rr = sqrt(1.0 / a - 1.0 / (a + b) +
                        1.0 / c - 1.0 / (c + d));
    double se_or = sqrt(1.0 / a + 1.0 / b + 1.0 / c + 1.0 / d);
    result->rr_lower = exp(log(result->relative_risk) - z * se_rr);
    result->rr_upper = exp(log(result->relative_risk) + z * se_rr);
    result->or_lower = exp(log(result->odds_ratio) - z * se_or);
    result->or_upper = exp(log(result->odds_ratio) + z * se_or);
    result->valid = isfinite(result->relative_risk) && isfinite(result->odds_ratio);
    if (!result->valid) {
        milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0,
                       "Riesgo relativo no finito");
        return MILENA_ERR_OVERFLOW;
    }
    return MILENA_OK;
}

MilenaStatus sst_mann_whitney_u(const double *first, size_t n_first,
                              const double *second, size_t n_second,
                              SstMannWhitneyResult *result,
                              MilenaError *error) {
    if (!first || !second || !result || n_first == 0 || n_second == 0) {
        return MILENA_ERR_ARGUMENT;
    }
    RankedValue *items = (RankedValue *)calloc(n_first + n_second, sizeof(*items));
    if (!items) return MILENA_ERR_MEMORY;
    for (size_t i = 0; i < n_first; i++) {
        if (!isfinite(first[i])) { free(items); return MILENA_ERR_TYPE; }
        items[i].value = first[i]; items[i].group = 0;
    }
    for (size_t i = 0; i < n_second; i++) {
        if (!isfinite(second[i])) { free(items); return MILENA_ERR_TYPE; }
        items[n_first + i].value = second[i]; items[n_first + i].group = 1;
    }
    size_t total = n_first + n_second;
    for (size_t i = 0; i < total; i++) {
        for (size_t j = i + 1; j < total; j++) {
            if (items[j].value < items[i].value) {
                RankedValue tmp = items[i]; items[i] = items[j]; items[j] = tmp;
            }
        }
    }
    double rank_sum_first = 0.0;
    for (size_t i = 0; i < total; i++) {
        double rank = (double)i + 1.0;
        size_t j = i + 1;
        while (j < total && items[j].value == items[i].value) j++;
        if (j > i + 1) rank = ((double)i + 1.0 + (double)j) / 2.0;
        if (items[i].group == 0) rank_sum_first += rank;
    }
    double u1 = rank_sum_first - (double)n_first * (double)(n_first + 1) / 2.0;
    double mean_u = (double)n_first * (double)n_second / 2.0;
    double variance_u = (double)n_first * (double)n_second * (double)(total + 1) / 12.0;
    result->n_first = n_first; result->n_second = n_second; result->u = u1;
    result->z = variance_u > 0.0 ? (u1 - mean_u) / sqrt(variance_u) : 0.0;
    result->p_value = two_sided_normal_p(result->z);
    result->approximate = true;
    free(items);
    (void)error;
    return MILENA_OK;
}

MilenaStatus sst_wilcoxon_signed_rank(const double *before, const double *after,
                                    size_t count, SstWilcoxonResult *result,
                                    MilenaError *error) {
    if (!before || !after || !result || count == 0) return MILENA_ERR_ARGUMENT;
    double *absolute = (double *)calloc(count, sizeof(*absolute));
    int *signs = (int *)calloc(count, sizeof(*signs));
    if (!absolute || !signs) { free(absolute); free(signs); return MILENA_ERR_MEMORY; }
    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        if (!isfinite(before[i]) || !isfinite(after[i])) continue;
        double difference = after[i] - before[i];
        if (difference == 0.0) continue;
        absolute[n] = fabs(difference);
        signs[n] = difference > 0.0 ? 1 : -1;
        n++;
    }
    if (n == 0) {
        free(absolute); free(signs);
        milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0,
                       "Sin diferencias no nulas para Wilcoxon");
        return MILENA_ERR_DATA;
    }
    double *sorted = (double *)malloc(n * sizeof(*sorted));
    if (!sorted) { free(absolute); free(signs); return MILENA_ERR_MEMORY; }
    memcpy(sorted, absolute, n * sizeof(*sorted));
    qsort(sorted, n, sizeof(*sorted), compare_double);
    double signed_rank = 0.0;
    for (size_t i = 0; i < n; i++) {
        size_t position = 0;
        while (position < n && sorted[position] != absolute[i]) position++;
        signed_rank += (double)(position + 1) * (double)signs[i];
    }
    double mean = (double)n * (double)(n + 1) / 4.0;
    double variance = (double)n * (double)(n + 1) * (double)(2 * n + 1) / 24.0;
    result->pairs = n;
    result->statistic = signed_rank;
    result->z = variance > 0.0 ? (signed_rank - mean) / sqrt(variance) : 0.0;
    result->p_value = two_sided_normal_p(result->z);
    result->approximate = true;
    free(sorted); free(absolute); free(signs);
    return MILENA_OK;
}

double sst_chi_square_approx_pvalue(double statistic, size_t degrees_of_freedom) {
    if (!isfinite(statistic) || statistic < 0.0 || degrees_of_freedom == 0) return 0.0;
    double df = (double)degrees_of_freedom;
    double z = (pow(statistic / df, 1.0 / 3.0) - (1.0 - 2.0 / (9.0 * df))) /
               sqrt(2.0 / (9.0 * df));
    return 1.0 - normal_cdf(z);
}
