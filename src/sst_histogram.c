#include "sst_histogram.h"

void sst_histogram_destroy(SstHistogram *histogram) {
    if (!histogram) return;
    free(histogram->counts);
    memset(histogram, 0, sizeof(*histogram));
}

ManoStatus sst_histogram_init(SstHistogram *histogram, size_t bin_count,
                              double minimum, double maximum,
                              ManoError *error) {
    if (!histogram || bin_count == 0 || !isfinite(minimum) ||
        !isfinite(maximum) || maximum <= minimum) {
        mano_error_set(error, MANO_ERR_ARGUMENT, 0, 0, 0,
                       "Límites de histograma inválidos");
        return MANO_ERR_ARGUMENT;
    }
    memset(histogram, 0, sizeof(*histogram));
    if (bin_count > SIZE_MAX / sizeof(*histogram->counts)) {
        mano_error_set(error, MANO_ERR_OVERFLOW, 0, 0, 0,
                       "Demasiados bins");
        return MANO_ERR_OVERFLOW;
    }
    histogram->counts = (size_t *)calloc(bin_count, sizeof(*histogram->counts));
    if (!histogram->counts) {
        mano_error_set(error, MANO_ERR_MEMORY, 0, 0, 0,
                       "Sin memoria para histograma");
        return MANO_ERR_MEMORY;
    }
    histogram->bin_count = bin_count;
    histogram->minimum = minimum;
    histogram->maximum = maximum;
    histogram->width = (maximum - minimum) / (double)bin_count;
    return MANO_OK;
}

ManoStatus sst_histogram_add(SstHistogram *histogram, double value,
                             ManoError *error) {
    if (!histogram || !histogram->counts) return MANO_ERR_ARGUMENT;
    if (!isfinite(value)) {
        histogram->invalid++;
        mano_error_set(error, MANO_ERR_TYPE, 0, 0, 0,
                       "Valor no finito para histograma SST");
        return MANO_ERR_TYPE;
    }
    if (value < histogram->minimum) {
        histogram->underflow++;
        return MANO_OK;
    }
    if (value > histogram->maximum) {
        histogram->overflow++;
        return MANO_OK;
    }
    size_t index = (value == histogram->maximum)
        ? histogram->bin_count - 1
        : (size_t)((value - histogram->minimum) / histogram->width);
    if (index >= histogram->bin_count) index = histogram->bin_count - 1;
    histogram->counts[index]++;
    return MANO_OK;
}

ManoStatus sst_histogram_add_text(SstHistogram *histogram, const char *text,
                                  ManoError *error) {
    double value;
    ManoStatus status = mano_parse_double(text, &value);
    if (status != MANO_OK) {
        if (histogram) histogram->invalid++;
        mano_error_set(error, MANO_ERR_TYPE, 0, 0, 0,
                       "Texto no convertible para histograma");
        return status;
    }
    return sst_histogram_add(histogram, value, error);
}
