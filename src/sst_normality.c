#include "sst_normality.h"

ManoStatus sst_normality_test(const double *data, size_t n,
                              SstNormalityResult *result,
                              ManoError *error) {
    if (!result || !data || n < 8) {
        if (result) {
            memset(result, 0, sizeof(*result));
            result->n = n;
            (void)snprintf(result->method, sizeof(result->method), "Jarque-Bera aproximado");
            (void)snprintf(result->interpretation, sizeof(result->interpretation),
                           "Muestra insuficiente; se requieren al menos 8 observaciones");
        }
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0,
                       "La prueba de normalidad requiere al menos 8 observaciones");
        return MANO_ERR_DATA;
    }
    memset(result, 0, sizeof(*result));
    result->n = n;
    result->approximate = true;
    (void)snprintf(result->method, sizeof(result->method), "Jarque-Bera aproximado");
    double mean = 0.0;
    size_t valid = 0;
    for (size_t i = 0; i < n; i++) {
        if (isfinite(data[i])) { mean += data[i]; valid++; }
    }
    if (valid < 8) {
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0,
                       "Muy pocos valores finitos para normalidad");
        return MANO_ERR_DATA;
    }
    mean /= (double)valid;
    double m2 = 0.0, m3 = 0.0, m4 = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (!isfinite(data[i])) continue;
        double d = data[i] - mean;
        double d2 = d * d;
        m2 += d2;
        m3 += d2 * d;
        m4 += d2 * d2;
    }
    double variance = m2 / (double)valid;
    if (variance <= DBL_EPSILON) {
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0,
                       "La variable es constante; no se puede evaluar normalidad");
        return MANO_ERR_DATA;
    }
    double skew = (m3 / (double)valid) / pow(variance, 1.5);
    double excess = (m4 / (double)valid) / (variance * variance) - 3.0;
    result->statistic = (double)valid / 6.0 * (skew * skew + excess * excess / 4.0);
    result->p_value = exp(-result->statistic / 2.0);
    result->normal = result->p_value >= 0.05;
    (void)snprintf(result->interpretation, sizeof(result->interpretation),
                   "Diagnóstico aproximado: JB=%.6g, p=%.6g; %s normalidad con alfa=0.05. No implica causalidad.",
                   result->statistic, result->p_value,
                   result->normal ? "no se rechaza" : "se rechaza");
    return MANO_OK;
}
