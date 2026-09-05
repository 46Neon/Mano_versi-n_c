#include "sst_report.h"

static void json_string(FILE *out, const char *text) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); *p; p++) {
        if (*p == '"') fputs("\\\"", out);
        else if (*p == '\\') fputs("\\\\", out);
        else if (*p == '\n') fputs("\\n", out);
        else if (*p == '\r') fputs("\\r", out);
        else if (*p == '\t') fputs("\\t", out);
        else if (*p < 0x20) fprintf(out, "\\u%04x", *p);
        else fputc(*p, out);
    }
    fputc('"', out);
}

ManoStatus sst_report_write_json(const char *filename,
                                 const SstEventList *events,
                                 const SstStats *severity,
                                 const SstHistogram *histogram,
                                 const SstRateResult *rate,
                                 ManoError *error) {
    if (!filename || !events || !severity || !histogram || !rate) return MANO_ERR_ARGUMENT;
    FILE *out = fopen(filename, "wb");
    if (!out) {
        mano_error_set(error, MANO_ERR_IO, 0, 0, 0, "No se pudo abrir reporte SST");
        return MANO_ERR_IO;
    }
    fprintf(out, "{\n  \"analisis\": \"sst\",\n");
    fprintf(out, "  \"eventos\": %zu,\n", events->count);
    fprintf(out, "  \"severidad\": {\"n\": %zu, \"invalidos\": %zu, "
                 "\"media\": %.10g, \"varianza_muestral\": %.10g, "
                 "\"desviacion\": %.10g, \"minimo\": %.10g, \"maximo\": %.10g},\n",
            severity->count, severity->invalid, severity->mean,
            sst_stats_variance_sample(severity),
            sst_stats_standard_deviation(severity),
            severity->has_value ? severity->minimum : 0.0,
            severity->has_value ? severity->maximum : 0.0);
    fprintf(out, "  \"histograma\": {\"bins\": [");
    for (size_t i = 0; i < histogram->bin_count; i++) {
        if (i) fputs(", ", out);
        double lower = histogram->minimum + histogram->width * (double)i;
        double upper = i + 1 == histogram->bin_count
            ? histogram->maximum
            : lower + histogram->width;
        fprintf(out, "{\"inferior\": %.10g, \"superior\": %.10g, \"cantidad\": %zu}",
                lower, upper, histogram->counts[i]);
    }
    fprintf(out, "], \"bajo_minimo\": %zu, \"sobre_maximo\": %zu, \"invalidos\": %zu},\n",
            histogram->underflow, histogram->overflow, histogram->invalid);
    fprintf(out, "  \"tasa\": {\"incidentes\": %zu, \"exposicion_horas\": %.10g, "
                 "\"factor\": %.10g, \"tasa\": %.10g, \"valida\": %s, "
                 "\"invalidos\": %zu}\n",
            rate->incident_count, rate->exposure_hours, rate->factor,
            rate->rate, rate->valid ? "true" : "false", rate->invalid_incidents);
    fputs("}\n", out);
    bool io_error = ferror(out) != 0;
    if (fclose(out) != 0) io_error = true;
    if (io_error) {
        mano_error_set(error, MANO_ERR_IO, 0, 0, 0,
                       "Error escribiendo reporte SST");
        return MANO_ERR_IO;
    }
    return MANO_OK;
}
