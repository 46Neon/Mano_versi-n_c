#ifndef MANO_SST_CORRELATION_H
#define MANO_SST_CORRELATION_H

#include "common.h"

typedef struct {
    size_t pairs;
    size_t invalid;
    double coefficient;
    bool valid;
    bool warning_small_sample;
} SstCorrelationResult;

ManoStatus sst_pearson(const double *x, const double *y, size_t count,
                       SstCorrelationResult *result, ManoError *error);

#endif
