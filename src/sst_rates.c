#include "sst_rates.h"

MilenaStatus sst_rate_from_counts(size_t incidents, double exposure_hours,
                                double factor, SstRateResult *result,
                                MilenaError *error) {
    if (!result || !isfinite(exposure_hours) || !isfinite(factor) ||
        exposure_hours <= 0.0 || factor <= 0.0) {
        if (result) memset(result, 0, sizeof(*result));
        milena_error_set(error, MILENA_ERR_DATA, 0, 0, 0,
                       "Exposición o factor de tasa inválido");
        return MILENA_ERR_DATA;
    }
    memset(result, 0, sizeof(*result));
    result->incident_count = incidents;
    result->exposure_hours = exposure_hours;
    result->factor = factor;
    result->rate = (double)incidents / exposure_hours * factor;
    result->valid = isfinite(result->rate);
    if (!result->valid) {
        milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0,
                       "Tasa SST no finita");
        return MILENA_ERR_OVERFLOW;
    }
    return MILENA_OK;
}

MilenaStatus sst_rate_from_events(const SstEventList *events, double factor,
                                SstRateResult *result, MilenaError *error) {
    if (!events || !result) return MILENA_ERR_ARGUMENT;
    size_t incidents = 0, invalid = 0;
    double exposure = 0.0;
    for (size_t i = 0; i < events->count; i++) {
        const SstEvent *event = &events->items[i];
        if (event->ocurrio_incidente == SST_BINARY_TRUE) incidents++;
        else if (event->ocurrio_incidente == SST_BINARY_INVALID) invalid++;
        if (event->has_horas_exposicion) {
            if (!isfinite(event->horas_exposicion) || event->horas_exposicion < 0.0) {
                invalid++;
                continue;
            }
            exposure += event->horas_exposicion;
        }
    }
    MilenaStatus status = sst_rate_from_counts(incidents, exposure, factor,
                                              result, error);
    result->invalid_incidents = invalid;
    return status;
}
