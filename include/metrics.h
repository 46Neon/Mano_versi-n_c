#ifndef MILENA_METRICS_H
#define MILENA_METRICS_H

#include "common.h"
#include <time.h>

typedef struct {
    size_t rows_read;
    size_t rows_valid;
    size_t rows_rejected;
    size_t operations;
    struct timespec started;
    struct timespec finished;
} MilenaMetrics;

void milena_metrics_init(MilenaMetrics *metrics);
void milena_metrics_start(MilenaMetrics *metrics);
void milena_metrics_finish(MilenaMetrics *metrics);
double milena_metrics_elapsed_seconds(const MilenaMetrics *metrics);

#endif
