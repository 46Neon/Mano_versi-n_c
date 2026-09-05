#ifndef MANO_SST_STATS_H
#define MANO_SST_STATS_H

#include "common.h"

typedef struct {
    size_t count;
    size_t invalid;
    double mean;
    double m2;
    double minimum;
    double maximum;
    bool has_value;
} SstStats;

void sst_stats_init(SstStats *stats);
ManoStatus sst_stats_add(SstStats *stats, double value, ManoError *error);
ManoStatus sst_stats_add_text(SstStats *stats, const char *text, ManoError *error);
ManoStatus sst_stats_merge(SstStats *destination, const SstStats *source,
                           ManoError *error);
double sst_stats_variance_sample(const SstStats *stats);
double sst_stats_standard_deviation(const SstStats *stats);

#endif
