#ifndef MANO_SST_ADVANCED_H
#define MANO_SST_ADVANCED_H

#include "common.h"

typedef struct {
    size_t count;
    size_t invalid;
    double total_weight;
    double mean;
    double variance;
    double standard_deviation;
    double coefficient_variation;
    double skewness;
    double excess_kurtosis;
    double p90;
    double p95;
    bool valid;
} SstAdvancedStats;

ManoStatus sst_advanced_compute(const double *values, const double *weights,
                                size_t count, SstAdvancedStats *result,
                                ManoError *error);

#endif
