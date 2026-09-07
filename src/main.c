#include "common.h"
#include "dataset.h"
#include "analysis.h"
#include "script.h"

static void usage(const char *program) {
    printf("Milena %s\n", MILENA_VERSION);
    printf("Uso:\n");
    printf("  %s analizar <csv> <json>\n", program);
    printf("  %s perfil <csv> <json>\n", program);
    printf("  %s run <archivo.milena>\n", program);
    printf("  %s inspect <csv>\n", program);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 2; }
    MilenaError error;
    milena_error_clear(&error);

    if (strcmp(argv[1], "analizar") == 0 && argc == 4) {
        Dataset dataset;
        dataset_init(&dataset);
        MilenaStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MILENA_OK) {
            dataset_print(&dataset, 5, stdout);
            SalesSummary summary;
            status = analysis_sales(&dataset, "fecha", "precio", "cantidad",
                                    argv[3], &summary, &error);
            if (status == MILENA_OK) {
                printf("Total: %.2f | Promedio: %.2f | Máxima: %.2f | Mínima: %.2f\n",
                       summary.total, summary.average, summary.maximum, summary.minimum);
                printf("Vistas: %zu | Usadas: %zu | Rechazadas: %zu\n",
                       summary.rows_seen, summary.rows_used, summary.rows_rejected);
            }
        }
        dataset_destroy(&dataset);
        if (status != MILENA_OK) { milena_error_print(&error, stderr); return 1; }
        return 0;
    }

    if (strcmp(argv[1], "perfil") == 0 && argc == 4) {
        Dataset dataset;
        MilenaSchema schema;
        dataset_init(&dataset);
        schema_init(&schema);
        MilenaStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MILENA_OK) {
            for (size_t i = 0; i < dataset.column_count; i++) {
                status = schema_add(&schema, dataset.headers[i], MILENA_VAR_TEXT,
                                    MILENA_ROLE_FEATURE, &error);
                if (status != MILENA_OK) break;
            }
        }
        if (status == MILENA_OK) {
            status = analysis_dataset_report(&dataset, &schema, argv[3], &error);
        }
        schema_destroy(&schema);
        dataset_destroy(&dataset);
        if (status != MILENA_OK) { milena_error_print(&error, stderr); return 1; }
        printf("Perfil guardado en: %s\n", argv[3]);
        return 0;
    }

    if (strcmp(argv[1], "run") == 0 && argc == 3) {
        MilenaStatus status = milena_run_script(argv[2], &error);
        if (status != MILENA_OK) { milena_error_print(&error, stderr); return 1; }
        return 0;
    }

    if (strcmp(argv[1], "inspect") == 0 && argc == 3) {
        Dataset dataset;
        dataset_init(&dataset);
        MilenaStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MILENA_OK) dataset_print(&dataset, 10, stdout);
        dataset_destroy(&dataset);
        if (status != MILENA_OK) { milena_error_print(&error, stderr); return 1; }
        return 0;
    }

    usage(argv[0]);
    return 2;
}
