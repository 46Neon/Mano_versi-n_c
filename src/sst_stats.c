#include "sst_stats.h"

void sst_stats_init(SstStats *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    stats->minimum = DBL_MAX;
    stats->maximum = -DBL_MAX;
}

ManoStatus sst_stats_add(SstStats *stats, double value, ManoError *error) {
    if (!stats) return MANO_ERR_ARGUMENT;
    if (!isfinite(value)) {
        stats->invalid++;
        mano_error_set(error, MANO_ERR_TYPE, 0, 0, 0,
                       "Valor SST no finito");
        return MANO_ERR_TYPE;
    }
    stats->count++;
    double delta = value - stats->mean;
    stats->mean += delta / (double)stats->count;
    double delta2 = value - stats->mean;
    stats->m2 += delta * delta2;
    if (!stats->has_value || value < stats->minimum) stats->minimum = value;
    if (!stats->has_value || value > stats->maximum) stats->maximum = value;
    stats->has_value = true;
    return MANO_OK;
}

ManoStatus sst_stats_add_text(SstStats *stats, const char *text, ManoError *error) {
    double value;
    ManoStatus status = mano_parse_double(text, &value);
    if (status != MANO_OK) {
        if (stats) stats->invalid++;
        mano_error_set(error, MANO_ERR_TYPE, 0, 0, 0,
                       "Texto no convertible a número SST");
        return status;
    }
    return sst_stats_add(stats, value, error);
}

ManoStatus sst_stats_merge(SstStats *destination, const SstStats *source,
                           ManoError *error) {
    if (!destination || !source) return MANO_ERR_ARGUMENT;
    if (source->count == 0) {
        destination->invalid += source->invalid;
        return MANO_OK;
    }
    if (destination->count == 0) {
        *destination = *source;
        return MANO_OK;
    }
    size_t total;
    if (source->count > SIZE_MAX - destination->count) {
        mano_error_set(error, MANO_ERR_OVERFLOW, 0, 0, 0,
                       "Overflow combinando estadísticas SST");
        return MANO_ERR_OVERFLOW;
    }
    total = destination->count + source->count;
    double delta = source->mean - destination->mean;
    destination->m2 += source->m2 + delta * delta *
                       ((double)destination->count * (double)source->count / (double)total);
    destination->mean += delta * (double)source->count / (double)total;
    destination->count = total;
    destination->invalid += source->invalid;
    if (source->minimum < destination->minimum) destination->minimum = source->minimum;
    if (source->maximum > destination->maximum) destination->maximum = source->maximum;
    destination->has_value = true;
    return MANO_OK;
}

double sst_stats_variance_sample(const SstStats *stats) {
    if (!stats || stats->count < 2) return 0.0;
    return stats->m2 / (double)(stats->count - 1);
}

double sst_stats_standard_deviation(const SstStats *stats) {
    double variance = sst_stats_variance_sample(stats);
    return variance > 0.0 ? sqrt(variance) : 0.0;
}
