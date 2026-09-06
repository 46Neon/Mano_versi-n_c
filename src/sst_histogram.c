#include "sst_histogram.h"

void sst_histogram_destroy(SstHistogram *histogram) {
    if (!histogram) return;
    free(histogram->counts);
    memset(histogram, 0, sizeof(*histogram));
}

MilenaStatus sst_histogram_init(SstHistogram *histogram, size_t bin_count,
                              double minimum, double maximum,
                              MilenaError *error) {
    if (!histogram || bin_count == 0 || !isfinite(minimum) ||
        !isfinite(maximum) || maximum <= minimum) {
        milena_error_set(error, MILENA_ERR_ARGUMENT, 0, 0, 0,
                       "Límites de histograma inválidos");
        return MILENA_ERR_ARGUMENT;
    }
    memset(histogram, 0, sizeof(*histogram));
    if (bin_count > SIZE_MAX / sizeof(*histogram->counts)) {
        milena_error_set(error, MILENA_ERR_OVERFLOW, 0, 0, 0,
                       "Demasiados bins");
        return MILENA_ERR_OVERFLOW;
    }
    histogram->counts = (size_t *)calloc(bin_count, sizeof(*histogram->counts));
    if (!histogram->counts) {
        milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, 0,
                       "Sin memoria para histograma");
        return MILENA_ERR_MEMORY;
    }
    histogram->bin_count = bin_count;
    histogram->minimum = minimum;
    histogram->maximum = maximum;
    histogram->width = (maximum - minimum) / (double)bin_count;
    return MILENA_OK;
}

MilenaStatus sst_histogram_add(SstHistogram *histogram, double value,
                             MilenaError *error) {
    if (!histogram || !histogram->counts) return MILENA_ERR_ARGUMENT;
    if (!isfinite(value)) {
        histogram->invalid++;
        milena_error_set(error, MILENA_ERR_TYPE, 0, 0, 0,
                       "Valor no finito para histograma SST");
        return MILENA_ERR_TYPE;
    }
    if (value < histogram->minimum) {
        histogram->underflow++;
        return MILENA_OK;
    }
    if (value > histogram->maximum) {
        histogram->overflow++;
        return MILENA_OK;
    }
    size_t index = (value == histogram->maximum)
        ? histogram->bin_count - 1
        : (size_t)((value - histogram->minimum) / histogram->width);
    if (index >= histogram->bin_count) index = histogram->bin_count - 1;
    histogram->counts[index]++;
    return MILENA_OK;
}

MilenaStatus sst_histogram_add_text(SstHistogram *histogram, const char *text,
                                  MilenaError *error) {
    double value;
    MilenaStatus status = milena_parse_double(text, &value);
    if (status != MILENA_OK) {
        if (histogram) histogram->invalid++;
        milena_error_set(error, MILENA_ERR_TYPE, 0, 0, 0,
                       "Texto no convertible para histograma");
        return status;
    }
    return sst_histogram_add(histogram, value, error);
}
