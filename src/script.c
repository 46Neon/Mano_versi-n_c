#include "script.h"
#include "analysis.h"
#include "sst_advanced.h"
#include "sst_contingency.h"
#include "sst_correlation.h"
#include "sst_histogram.h"
#include "sst_inference.h"
#include "sst_model.h"
#include "sst_normality.h"
#include "sst_rates.h"

static char *read_file(const char *filename, ManoError *error) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        mano_error_set(error, MANO_ERR_IO, 0, 0, 0, "No se pudo abrir script");
        return NULL;
    }
    size_t cap = 4096, len = 0;
    char *text = (char *)malloc(cap);
    if (!text) { fclose(file); return NULL; }
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (len + 1 >= cap) {
            if (cap > SIZE_MAX / 2) { free(text); fclose(file); return NULL; }
            cap *= 2;
            char *tmp = (char *)realloc(text, cap);
            if (!tmp) { free(text); fclose(file); return NULL; }
            text = tmp;
        }
        text[len++] = (char)ch;
    }
    fclose(file);
    text[len] = '\0';
    return text;
}

static char *trim_left(char *text) {
    while (*text == ' ' || *text == '\t') text++;
    return text;
}

static const char *find_quoted_after(const char *text, const char *needle,
                                     char *out, size_t out_size) {
    const char *p = strstr(text, needle);
    if (!p) return NULL;
    p = strchr(p, '"');
    if (!p) return NULL;
    p++;
    const char *end = strchr(p, '"');
    if (!end || (size_t)(end - p) + 1 > out_size) return NULL;
    memcpy(out, p, (size_t)(end - p));
    out[end - p] = '\0';
    return end + 1;
}

static bool get_quoted(const char *line, size_t number, char *out, size_t out_size) {
    const char *p = line;
    for (size_t i = 0; i <= number; i++) {
        p = strchr(p, '"');
        if (!p) return false;
        p++;
        const char *end = strchr(p, '"');
        if (!end) return false;
        if (i == number) {
            if ((size_t)(end - p) + 1 > out_size) return false;
            memcpy(out, p, (size_t)(end - p));
            out[end - p] = '\0';
            return true;
        }
        p = end + 1;
    }
    return false;
}

static bool has_text(const char *text, const char *needle) {
    return text && needle && strstr(text, needle) != NULL;
}

static ManoVariableType parse_type(const char *text, bool *valid) {
    *valid = true;
    if (strcmp(text, "numerica") == 0 || strcmp(text, "numeric") == 0) return MANO_VAR_NUMERIC;
    if (strcmp(text, "categorica") == 0 || strcmp(text, "categorical") == 0) return MANO_VAR_CATEGORICAL;
    if (strcmp(text, "binaria") == 0 || strcmp(text, "binary") == 0) return MANO_VAR_BINARY;
    if (strcmp(text, "fecha") == 0 || strcmp(text, "date") == 0) return MANO_VAR_TEXT;
    if (strcmp(text, "texto") == 0 || strcmp(text, "text") == 0) return MANO_VAR_TEXT;
    *valid = false;
    return MANO_VAR_TEXT;
}

static ManoStatus parse_schema(const char *script, ManoSchema *schema, ManoError *error) {
    char *copy = mano_strdup(script);
    if (!copy) return MANO_ERR_MEMORY;
    char *line = strtok(copy, "\n\r");
    size_t line_number = 0;
    while (line) {
        line_number++;
        char *text = trim_left(line);
        if (strncmp(text, "variable ", 9) == 0) {
            char name[256], type_name[64];
            if (sscanf(text + 9, "%255s %63s", name, type_name) != 2) {
                mano_error_set(error, MANO_ERR_PARSE, line_number, 1, 0,
                               "Sintaxis: variable nombre tipo");
                free(copy); return MANO_ERR_PARSE;
            }
            bool valid;
            ManoVariableType type = parse_type(type_name, &valid);
            if (!valid) {
                mano_error_set(error, MANO_ERR_TYPE, line_number, 1, 0,
                               "Tipo de variable desconocido");
                free(copy); return MANO_ERR_TYPE;
            }
            ManoStatus status = schema_add(schema, name, type, MANO_ROLE_FEATURE, error);
            if (status != MANO_OK) { free(copy); return status; }
        } else if (strncmp(text, "entrada categorica", 18) == 0) {
            char name[256];
            if (!find_quoted_after(text, "entrada categorica", name, sizeof(name))) {
                mano_error_set(error, MANO_ERR_PARSE, line_number, 1, 0,
                               "Sintaxis: entrada categorica \"columna\"");
                free(copy); return MANO_ERR_PARSE;
            }
            ManoStatus status = schema_add(schema, name, MANO_VAR_CATEGORICAL,
                                           MANO_ROLE_CATEGORICAL_INPUT, error);
            if (status != MANO_OK) { free(copy); return status; }
        } else if (strncmp(text, "salida binaria", 14) == 0) {
            char name[256];
            if (!find_quoted_after(text, "salida binaria", name, sizeof(name))) {
                mano_error_set(error, MANO_ERR_PARSE, line_number, 1, 0,
                               "Sintaxis: salida binaria \"columna\"");
                free(copy); return MANO_ERR_PARSE;
            }
            ManoStatus status = schema_add(schema, name, MANO_VAR_BINARY,
                                           MANO_ROLE_BINARY_OUTPUT, error);
            if (status != MANO_OK) { free(copy); return status; }
        }
        line = strtok(NULL, "\n\r");
    }
    free(copy);
    if (schema->count == 0) {
        mano_error_set(error, MANO_ERR_PARSE, 0, 0, 0,
                       "El script debe declarar al menos una variable");
        return MANO_ERR_PARSE;
    }
    return MANO_OK;
}

static bool command_known(const char *text) {
    static const char *known[] = {
        "#datos", "#estadistica", "#nulos", "#duplicados", "#total",
        "#periodo", "#condicion", "#perfil_numerico", "#perfil_avanzado",
        "#histograma", "#normalidad", "#balance", "#tasa", "#poisson", "#correlacion",
        "#chi_cuadrado"
    };
    for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
        if (strncmp(text, known[i], strlen(known[i])) == 0) return true;
    }
    return false;
}

static ManoStatus validate_commands(const char *script, ManoError *error) {
    char *copy = mano_strdup(script);
    if (!copy) return MANO_ERR_MEMORY;
    char *line = strtok(copy, "\n\r");
    size_t line_number = 0;
    while (line) {
        line_number++;
        char *text = trim_left(line);
        if (text[0] == '#') {
            if (!command_known(text)) {
                mano_error_set(error, MANO_ERR_UNSUPPORTED, line_number, 1, 0,
                               "Comando Mano no reconocido; no se ignorará silenciosamente");
                free(copy);
                return MANO_ERR_UNSUPPORTED;
            }
        }
        line = strtok(NULL, "\n\r");
    }
    free(copy);
    return MANO_OK;
}

static ManoStatus numeric_column(const Dataset *dataset, const char *name,
                                 double **values, size_t *count, ManoError *error) {
    int index = dataset_column_index(dataset, name);
    if (index < 0) {
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0, "Columna numérica inexistente");
        return MANO_ERR_DATA;
    }
    double *result = (double *)calloc(dataset->row_count, sizeof(*result));
    if (dataset->row_count && !result) return MANO_ERR_MEMORY;
    size_t used = 0;
    for (size_t r = 0; r < dataset->row_count; r++) {
        double value;
        if (mano_parse_double(dataset->rows[r][index], &value) == MANO_OK) {
            result[used++] = value;
        }
    }
    if (used == 0) {
        free(result);
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0, "Columna sin valores numéricos válidos");
        return MANO_ERR_DATA;
    }
    *values = result;
    *count = used;
    return MANO_OK;
}

static ManoStatus paired_columns(const Dataset *dataset, const char *left,
                                 const char *right, double **x, double **y,
                                 size_t *count, ManoError *error) {
    int li = dataset_column_index(dataset, left);
    int ri = dataset_column_index(dataset, right);
    if (li < 0 || ri < 0) return MANO_ERR_DATA;
    double *xx = (double *)calloc(dataset->row_count, sizeof(*xx));
    double *yy = (double *)calloc(dataset->row_count, sizeof(*yy));
    if ((dataset->row_count && !xx) || (dataset->row_count && !yy)) {
        free(xx); free(yy); return MANO_ERR_MEMORY;
    }
    size_t used = 0;
    for (size_t r = 0; r < dataset->row_count; r++) {
        double a, b;
        if (mano_parse_double(dataset->rows[r][li], &a) == MANO_OK &&
            mano_parse_double(dataset->rows[r][ri], &b) == MANO_OK) {
            xx[used] = a; yy[used] = b; used++;
        }
    }
    if (used < 3) {
        free(xx); free(yy);
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0, "Pocos pares numéricos válidos");
        return MANO_ERR_DATA;
    }
    *x = xx; *y = yy; *count = used;
    return MANO_OK;
}

static void json_text(FILE *out, const char *text) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); *p; p++) {
        if (*p == '"') fputs("\\\"", out);
        else if (*p == '\\') fputs("\\\\", out);
        else if (*p == '\n') fputs("\\n", out);
        else fputc(*p, out);
    }
    fputc('"', out);
}

static ManoStatus run_sst_commands(const char *script, const Dataset *dataset,
                                   const char *output, ManoError *error) {
    char path[1200];
    int written = snprintf(path, sizeof(path), "%s.sst.json", output);
    if (written < 0 || (size_t)written >= sizeof(path)) return MANO_ERR_OVERFLOW;
    FILE *report = fopen(path, "wb");
    if (!report) return MANO_ERR_IO;
    fprintf(report, "{\n  \"analisis\": \"sst_comandos\",\n  \"proposito\": \"apoyo_preventivo_sst\",\n  \"determina_causalidad\": false,\n  \"requiere_revision_profesional\": true,\n  \"operaciones\": [\n");
    bool first = true;
    char *copy = mano_strdup(script);
    if (!copy) { fclose(report); return MANO_ERR_MEMORY; }
    char *line = strtok(copy, "\n\r");
    while (line) {
        char *text = trim_left(line);
        ManoStatus status = MANO_OK;
        if (strncmp(text, "#perfil_avanzado", 16) == 0 ||
            strncmp(text, "#perfil_numerico", 16) == 0) {
            char column[256];
            if (!get_quoted(text, 0, column, sizeof(column))) status = MANO_ERR_PARSE;
            double *values = NULL; size_t count = 0;
            if (status == MANO_OK) status = numeric_column(dataset, column, &values, &count, error);
            SstAdvancedStats stats;
            if (status == MANO_OK) status = sst_advanced_compute(values, NULL, count, &stats, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fputs("    {\"operacion\": \"perfil_avanzado\", \"variable\": ", report);
                json_text(report, column);
                fprintf(report, ", \"n\": %zu, \"invalidos\": %zu, \"media\": %.10g, \"desviacion\": %.10g, \"cv\": %.10g, \"asimetria\": %.10g, \"kurtosis_exceso\": %.10g, \"p90\": %.10g, \"p95\": %.10g}",
                        stats.count, stats.invalid, stats.mean, stats.standard_deviation,
                        stats.coefficient_variation, stats.skewness, stats.excess_kurtosis,
                        stats.p90, stats.p95);
            }
            free(values);
        } else if (strncmp(text, "#histograma", 11) == 0) {
            char column[256];
            if (!get_quoted(text, 0, column, sizeof(column))) status = MANO_ERR_PARSE;
            size_t bins = 5;
            const char *bp = strstr(text, "bins");
            if (bp) { const char *eq = strchr(bp, '='); if (eq) bins = (size_t)strtoul(eq + 1, NULL, 10); }
            double *values = NULL; size_t count = 0;
            if (status == MANO_OK) status = numeric_column(dataset, column, &values, &count, error);
            SstAdvancedStats stats;
            SstHistogram histogram;
            if (status == MANO_OK) status = sst_advanced_compute(values, NULL, count, &stats, error);
            if (status == MANO_OK) status = sst_histogram_init(&histogram, bins, stats.minimum, stats.maximum, error);
            if (status == MANO_OK) for (size_t i = 0; i < count; i++) (void)sst_histogram_add(&histogram, values[i], error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fprintf(report, "    {\"operacion\": \"histograma\", \"variable\": "); json_text(report, column);
                fprintf(report, ", \"bins\": [");
                for (size_t i = 0; i < histogram.bin_count; i++) {
                    if (i) fputs(", ", report);
                    fprintf(report, "%zu", histogram.counts[i]);
                }
                fprintf(report, "], \"bajo_minimo\": %zu, \"sobre_maximo\": %zu}", histogram.underflow, histogram.overflow);
            }
            if (status == MANO_OK) sst_histogram_destroy(&histogram);
            free(values);
        } else if (strncmp(text, "#normalidad", 11) == 0) {
            char column[256];
            if (!get_quoted(text, 0, column, sizeof(column))) status = MANO_ERR_PARSE;
            double *values = NULL; size_t count = 0;
            if (status == MANO_OK) status = numeric_column(dataset, column, &values, &count, error);
            SstNormalityResult normality;
            if (status == MANO_OK) status = sst_normality_test(values, count, &normality, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fputs("    {\"operacion\": \"normalidad\", \"variable\": ", report); json_text(report, column);
                fprintf(report, ", \"metodo\": "); json_text(report, normality.method);
                fprintf(report, ", \"estadistico\": %.10g, \"p\": %.10g, \"normal\": %s, \"aproximado\": %s, \"interpretacion\": ",
                        normality.statistic, normality.p_value,
                        normality.normal ? "true" : "false",
                        normality.approximate ? "true" : "false");
                json_text(report, normality.interpretation);
                fputs("}", report);
            }
            free(values);
        } else if (strncmp(text, "#poisson", 8) == 0) {
            char event_column[256], exposure_column[256];
            if (!get_quoted(text, 0, event_column, sizeof(event_column)) ||
                !get_quoted(text, 1, exposure_column, sizeof(exposure_column))) status = MANO_ERR_PARSE;
            double factor = 200000.0;
            const char *factor_text = strstr(text, "factor");
            if (factor_text) { const char *equal = strchr(factor_text, '='); if (equal) factor = strtod(equal + 1, NULL); }
            int event_index = dataset_column_index(dataset, event_column);
            int exposure_index = dataset_column_index(dataset, exposure_column);
            size_t incidents = 0; double exposure = 0.0;
            if (status == MANO_OK && (event_index < 0 || exposure_index < 0)) status = MANO_ERR_DATA;
            if (status == MANO_OK) for (size_t i = 0; i < dataset->row_count; i++) {
                if (sst_binary_parse(dataset->rows[i][event_index]) == SST_BINARY_TRUE) incidents++;
                double hours; if (mano_parse_double(dataset->rows[i][exposure_index], &hours) == MANO_OK && hours >= 0.0) exposure += hours;
            }
            SstPoissonInterval interval;
            if (status == MANO_OK) status = sst_poisson_exact_interval(incidents, exposure, factor, 0.95, &interval, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fputs("    {\"operacion\": \"poisson\", \"evento\": ", report); json_text(report, event_column);
                fputs(", \"exposicion\": ", report); json_text(report, exposure_column);
                fprintf(report, ", \"eventos\": %zu, \"tasa\": %.10g, \"ic_inferior\": %.10g, \"ic_superior\": %.10g, \"nivel\": 0.95, \"aproximado\": %s}", incidents, interval.rate, interval.lower, interval.upper, interval.approximate ? "true" : "false");
            }
        } else if (strncmp(text, "#tasa", 5) == 0) {
            char event_column[256], exposure_column[256];
            if (!get_quoted(text, 0, event_column, sizeof(event_column)) ||
                !get_quoted(text, 1, exposure_column, sizeof(exposure_column))) {
                status = MANO_ERR_PARSE;
            }
            double factor = 200000.0;
            const char *factor_text = strstr(text, "factor");
            if (factor_text) {
                const char *equal = strchr(factor_text, '=');
                if (equal) factor = strtod(equal + 1, NULL);
            }
            int event_index = dataset_column_index(dataset, event_column);
            int exposure_index = dataset_column_index(dataset, exposure_column);
            size_t incidents = 0; double exposure = 0.0;
            if (status == MANO_OK && (event_index < 0 || exposure_index < 0)) status = MANO_ERR_DATA;
            if (status == MANO_OK) {
                for (size_t i = 0; i < dataset->row_count; i++) {
                    SstBinaryValue binary = sst_binary_parse(dataset->rows[i][event_index]);
                    if (binary == SST_BINARY_TRUE) incidents++;
                    double hours;
                    if (mano_parse_double(dataset->rows[i][exposure_index], &hours) == MANO_OK && hours >= 0.0) exposure += hours;
                }
            }
            SstRateResult rate;
            if (status == MANO_OK) status = sst_rate_from_counts(incidents, exposure, factor, &rate, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fputs("    {\"operacion\": \"tasa\", \"evento\": ", report); json_text(report, event_column);
                fputs(", \"exposicion\": ", report); json_text(report, exposure_column);
                fprintf(report, ", \"incidentes\": %zu, \"horas\": %.10g, \"factor\": %.10g, \"tasa\": %.10g}", incidents, exposure, factor, rate.rate);
            }
        } else if (strncmp(text, "#correlacion", 12) == 0) {
            char left[256], right[256];
            if (!get_quoted(text, 0, left, sizeof(left)) || !get_quoted(text, 1, right, sizeof(right))) status = MANO_ERR_PARSE;
            double *x = NULL, *y = NULL; size_t count = 0;
            if (status == MANO_OK) status = paired_columns(dataset, left, right, &x, &y, &count, error);
            SstCorrelationResult result;
            if (status == MANO_OK) status = sst_pearson(x, y, count, &result, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fprintf(report, "    {\"operacion\": \"pearson\", \"x\": "); json_text(report, left);
                fprintf(report, ", \"y\": "); json_text(report, right);
                fprintf(report, ", \"pares\": %zu, \"r\": %.10g, \"advertencia_muestra_pequena\": %s}", result.pairs, result.coefficient, result.warning_small_sample ? "true" : "false");
            }
            free(x); free(y);
        } else if (strncmp(text, "#chi_cuadrado", 13) == 0) {
            char row_name[256], col_name[256];
            if (!get_quoted(text, 0, row_name, sizeof(row_name)) || !get_quoted(text, 1, col_name, sizeof(col_name))) status = MANO_ERR_PARSE;
            int ri = dataset_column_index(dataset, row_name), ci = dataset_column_index(dataset, col_name);
            const char **rows = NULL, **cols = NULL;
            if (status == MANO_OK && (ri < 0 || ci < 0)) status = MANO_ERR_DATA;
            if (status == MANO_OK) {
                rows = (const char **)calloc(dataset->row_count, sizeof(*rows));
                cols = (const char **)calloc(dataset->row_count, sizeof(*cols));
                if (!rows || !cols) status = MANO_ERR_MEMORY;
            }
            if (status == MANO_OK) for (size_t i = 0; i < dataset->row_count; i++) { rows[i] = dataset->rows[i][ri]; cols[i] = dataset->rows[i][ci]; }
            SstContingency2D table; sst_contingency_init(&table); SstChiSquareResult chi;
            if (status == MANO_OK) status = sst_contingency_build(rows, cols, dataset->row_count, &table, error);
            if (status == MANO_OK) status = sst_contingency_chi_square(&table, &chi, error);
            if (status == MANO_OK) {
                if (!first) fputs(",\n", report); first = false;
                fprintf(report, "    {\"operacion\": \"chi_cuadrado\", \"filas\": %zu, \"columnas\": %zu, \"estadistico\": %.10g, \"grados_libertad\": %zu, \"p_aproximado\": %.10g, \"celdas_esperadas_bajas\": %zu}", table.row_count, table.column_count, chi.statistic, chi.degrees_of_freedom, sst_chi_square_approx_pvalue(chi.statistic, chi.degrees_of_freedom), chi.low_expected_cells);
            }
            sst_contingency_destroy(&table); free(rows); free(cols);
        } else if (strncmp(text, "#balance", 8) == 0) {
            char column[256];
            if (!get_quoted(text, 0, column, sizeof(column))) status = MANO_ERR_PARSE;
            int index = dataset_column_index(dataset, column); size_t zeros = 0, ones = 0, invalid = 0;
            if (status == MANO_OK && index < 0) status = MANO_ERR_DATA;
            if (status == MANO_OK) for (size_t i = 0; i < dataset->row_count; i++) { SstBinaryValue v = sst_binary_parse(dataset->rows[i][index]); if (v == SST_BINARY_TRUE) ones++; else if (v == SST_BINARY_FALSE) zeros++; else invalid++; }
            if (status == MANO_OK) { if (!first) fputs(",\n", report); first = false; fprintf(report, "    {\"operacion\": \"balance\", \"variable\": "); json_text(report, column); fprintf(report, ", \"ceros\": %zu, \"unos\": %zu, \"invalidos\": %zu}", zeros, ones, invalid); }
        }
        if (status != MANO_OK) { free(copy); fclose(report); return status; }
        line = strtok(NULL, "\n\r");
    }
    free(copy);
    fputs("\n  ],\n  \"advertencias\": [\n", report);
    fputs("    {\"tipo\": \"causalidad\", \"mensaje\": ", report);
    json_text(report, "Asociación estadística no implica causalidad; pueden existir confusores, sesgo de selección o azar.");
    fputs("},\n", report);
    fputs("    {\"tipo\": \"aproximacion\", \"mensaje\": ", report);
    json_text(report, "Los métodos inferenciales aproximados deben interpretarse junto con sus supuestos y tamaño muestral.");
    fputs("}\n  ]\n}\n", report);
    bool io_error = ferror(report) != 0; if (fclose(report) != 0) io_error = true;
    if (io_error) return MANO_ERR_IO;
    printf("Reporte SST avanzado: %s\n", path);
    return MANO_OK;
}

ManoStatus mano_run_script(const char *filename, ManoError *error) {
    if (!filename) return MANO_ERR_ARGUMENT;
    char *script = read_file(filename, error);
    if (!script) return error && error->code ? error->code : MANO_ERR_IO;
    ManoSchema schema; schema_init(&schema);
    ManoStatus status = parse_schema(script, &schema, error);
    if (status == MANO_OK) status = validate_commands(script, error);
    if (status != MANO_OK) { free(script); schema_destroy(&schema); return status; }

    char input[1024] = {0}; char output[1024] = "reporte_dataset.json";
    if (!find_quoted_after(script, "dataset cargar", input, sizeof(input))) {
        free(script); schema_destroy(&schema); mano_error_set(error, MANO_ERR_PARSE, 0, 0, 0, "Falta dataset cargar datos(\"...\")"); return MANO_ERR_PARSE;
    }
    const char *export_pos = strstr(script, ".exportar");
    if (export_pos) (void)find_quoted_after(export_pos, ".exportar", output, sizeof(output));
    if (has_text(script, "#total(\"precio * cantidad\")") && schema_index(&schema, "total") < 0) status = schema_add(&schema, "total", MANO_VAR_NUMERIC, MANO_ROLE_FEATURE, error);
    if (status == MANO_OK && has_text(script, "#periodo extraer(\"mes de fecha\")") && schema_index(&schema, "periodo") < 0) status = schema_add(&schema, "periodo", MANO_VAR_CATEGORICAL, MANO_ROLE_FEATURE, error);

    Dataset dataset; dataset_init(&dataset); DatasetLimits limits = dataset_default_limits();
    if (status == MANO_OK) status = dataset_load_csv_with_limits(&dataset, input, ',', &limits, error);
    if (status == MANO_OK && has_text(script, "#nulos(\"eliminar\")")) status = dataset_remove_null_rows(&dataset, error);
    if (status == MANO_OK && has_text(script, "#duplicados(\"eliminar\")")) status = dataset_remove_duplicates(&dataset, error);
    if (status == MANO_OK && has_text(script, "#total(\"precio * cantidad\")")) status = dataset_add_product(&dataset, "precio", "cantidad", "total", error);
    if (status == MANO_OK && has_text(script, "#periodo extraer(\"mes de fecha\")")) status = dataset_add_month(&dataset, "fecha", "periodo", error);
    if (status == MANO_OK && has_text(script, "#condicion(\"total > 0\")")) status = dataset_filter_positive_product(&dataset, "precio", "cantidad", error);
    if (status == MANO_OK) status = analysis_dataset_report(&dataset, &schema, output, error);
    if (status == MANO_OK) status = run_sst_commands(script, &dataset, output, error);
    if (status == MANO_OK) { printf("Script ejecutado correctamente: %s\n", filename); printf("Filas: %zu | Columnas: %zu | Filas inválidas: %zu\n", dataset.row_count, dataset.column_count, dataset.invalid_rows); }
    dataset_destroy(&dataset); schema_destroy(&schema); free(script); return status;
}
