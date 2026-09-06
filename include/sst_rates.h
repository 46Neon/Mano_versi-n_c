#ifndef MILENA_SST_RATES_H
#define MILENA_SST_RATES_H

#include "common.h"
#include "sst_model.h"

typedef struct {
    size_t incident_count;
    size_t invalid_incidents;
    double exposure_hours;
    double factor;
    double rate;
    bool valid;
} SstRateResult;

MilenaStatus sst_rate_from_events(const SstEventList *events, double factor,
                                SstRateResult *result, MilenaError *error);
MilenaStatus sst_rate_from_counts(size_t incidents, double exposure_hours,
                                double factor, SstRateResult *result,
                                MilenaError *error);

#endif
