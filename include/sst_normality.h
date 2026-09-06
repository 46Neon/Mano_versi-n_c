#ifndef MILENA_SST_NORMALITY_H
#define MILENA_SST_NORMALITY_H

#include "common.h"

typedef struct {
    double statistic;
    double p_value;
    bool normal;
    bool approximate;
    size_t n;
    char method[64];
    char interpretation[256];
} SstNormalityResult;

/* Diagnóstico Jarque-Bera con aproximación chi-cuadrado de 2 grados de libertad. */
MilenaStatus sst_normality_test(const double *data, size_t n,
                              SstNormalityResult *result,
                              MilenaError *error);

#endif
