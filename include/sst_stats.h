#ifndef MILENA_SST_STATS_H
#define MILENA_SST_STATS_H

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
MilenaStatus sst_stats_add(SstStats *stats, double value, MilenaError *error);
MilenaStatus sst_stats_add_text(SstStats *stats, const char *text, MilenaError *error);
MilenaStatus sst_stats_merge(SstStats *destination, const SstStats *source,
                           MilenaError *error);
double sst_stats_variance_sample(const SstStats *stats);
double sst_stats_standard_deviation(const SstStats *stats);

#endif
