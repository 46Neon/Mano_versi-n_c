#ifndef MANO_SST_HISTOGRAM_H
#define MANO_SST_HISTOGRAM_H

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

ManoStatus sst_histogram_init(SstHistogram *histogram, size_t bin_count,
                              double minimum, double maximum,
                              ManoError *error);
void sst_histogram_destroy(SstHistogram *histogram);
ManoStatus sst_histogram_add(SstHistogram *histogram, double value,
                             ManoError *error);
ManoStatus sst_histogram_add_text(SstHistogram *histogram, const char *text,
                                  ManoError *error);

#endif
