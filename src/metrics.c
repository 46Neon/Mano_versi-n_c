#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "metrics.h"

#ifndef _WIN32
static void milena_metrics_now(struct timespec *value) {
    if (!value) return;
    if (clock_gettime(CLOCK_MONOTONIC, value) != 0) {
        value->tv_sec = time(NULL);
        value->tv_nsec = 0;
    }
}
#else
static void milena_metrics_now(struct timespec *value) {
    if (!value) return;
    (void)timespec_get(value, TIME_UTC);
}
#endif

void milena_metrics_init(MilenaMetrics *metrics) {
    if (!metrics) return;
    memset(metrics, 0, sizeof(*metrics));
}

void milena_metrics_start(MilenaMetrics *metrics) {
    if (!metrics) return;
    milena_metrics_now(&metrics->started);
}

void milena_metrics_finish(MilenaMetrics *metrics) {
    if (!metrics) return;
    milena_metrics_now(&metrics->finished);
}

double milena_metrics_elapsed_seconds(const MilenaMetrics *metrics) {
    if (!metrics) return 0.0;
    time_t seconds = metrics->finished.tv_sec - metrics->started.tv_sec;
    long nanoseconds = metrics->finished.tv_nsec - metrics->started.tv_nsec;
    return (double)seconds + (double)nanoseconds / 1000000000.0;
}
