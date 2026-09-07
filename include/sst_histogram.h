#ifndef MILENA_SST_HISTOGRAM_H
#define MILENA_SST_HISTOGRAM_H

#include "common.h"

typedef struct {
    size_t bin_count;
    double minimum;
    double maximum;
    double width;
    size_t *counts;
    size_t underflow;
    size_t overflow;
    size_t invalid;
} SstHistogram;

MilenaStatus sst_histogram_init(SstHistogram *histogram, size_t bin_count,
                              double minimum, double maximum,
                              MilenaError *error);
void sst_histogram_destroy(SstHistogram *histogram);
MilenaStatus sst_histogram_add(SstHistogram *histogram, double value,
                             MilenaError *error);
MilenaStatus sst_histogram_add_text(SstHistogram *histogram, const char *text,
                                  MilenaError *error);

#endif
