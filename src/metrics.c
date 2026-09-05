#include "metrics.h"

void mano_metrics_init(ManoMetrics *metrics) {
    if (!metrics) return;
    memset(metrics, 0, sizeof(*metrics));
}

void mano_metrics_start(ManoMetrics *metrics) {
    if (!metrics) return;
    (void)timespec_get(&metrics->started, TIME_UTC);
}

void mano_metrics_finish(ManoMetrics *metrics) {
    if (!metrics) return;
    (void)timespec_get(&metrics->finished, TIME_UTC);
}

double mano_metrics_elapsed_seconds(const ManoMetrics *metrics) {
    if (!metrics) return 0.0;
    time_t seconds = metrics->finished.tv_sec - metrics->started.tv_sec;
    long nanoseconds = metrics->finished.tv_nsec - metrics->started.tv_nsec;
    return (double)seconds + (double)nanoseconds / 1000000000.0;
}
