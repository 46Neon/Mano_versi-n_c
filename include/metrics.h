#ifndef MANO_METRICS_H
#define MANO_METRICS_H

#include "common.h"
#include <time.h>

typedef struct {
    size_t rows_read;
    size_t rows_valid;
    size_t rows_rejected;
    size_t operations;
    struct timespec started;
    struct timespec finished;
} ManoMetrics;

void mano_metrics_init(ManoMetrics *metrics);
void mano_metrics_start(ManoMetrics *metrics);
void mano_metrics_finish(ManoMetrics *metrics);
double mano_metrics_elapsed_seconds(const ManoMetrics *metrics);

#endif
