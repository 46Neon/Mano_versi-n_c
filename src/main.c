#include "common.h"
#include "dataset.h"
#include "analysis.h"
#include "script.h"

static void usage(const char *program) {
    printf("Mano %s\n", MANO_VERSION);
    printf("Uso:\n");
    printf("  %s analizar <csv> <json>\n", program);
    printf("  %s perfil <csv> <json>\n", program);
    printf("  %s run <archivo.mano>\n", program);
    printf("  %s inspect <csv>\n", program);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 2; }
    ManoError error;
    mano_error_clear(&error);

    if (strcmp(argv[1], "analizar") == 0 && argc == 4) {
        Dataset dataset;
        dataset_init(&dataset);
        ManoStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MANO_OK) {
            dataset_print(&dataset, 5, stdout);
            SalesSummary summary;
            status = analysis_sales(&dataset, "fecha", "precio", "cantidad",
                                    argv[3], &summary, &error);
            if (status == MANO_OK) {
                printf("Total: %.2f | Promedio: %.2f | Máxima: %.2f | Mínima: %.2f\n",
                       summary.total, summary.average, summary.maximum, summary.minimum);
                printf("Vistas: %zu | Usadas: %zu | Rechazadas: %zu\n",
                       summary.rows_seen, summary.rows_used, summary.rows_rejected);
            }
        }
        dataset_destroy(&dataset);
        if (status != MANO_OK) { mano_error_print(&error, stderr); return 1; }
        return 0;
    }

    if (strcmp(argv[1], "perfil") == 0 && argc == 4) {
        Dataset dataset;
        ManoSchema schema;
        dataset_init(&dataset);
        schema_init(&schema);
        ManoStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MANO_OK) {
            for (size_t i = 0; i < dataset.column_count; i++) {
                status = schema_add(&schema, dataset.headers[i], MANO_VAR_TEXT,
                                    MANO_ROLE_FEATURE, &error);
                if (status != MANO_OK) break;
            }
        }
        if (status == MANO_OK) {
            status = analysis_dataset_report(&dataset, &schema, argv[3], &error);
        }
        schema_destroy(&schema);
        dataset_destroy(&dataset);
        if (status != MANO_OK) { mano_error_print(&error, stderr); return 1; }
        printf("Perfil guardado en: %s\n", argv[3]);
        return 0;
    }

    if (strcmp(argv[1], "run") == 0 && argc == 3) {
        ManoStatus status = mano_run_script(argv[2], &error);
        if (status != MANO_OK) { mano_error_print(&error, stderr); return 1; }
        return 0;
    }

    if (strcmp(argv[1], "inspect") == 0 && argc == 3) {
        Dataset dataset;
        dataset_init(&dataset);
        ManoStatus status = dataset_load_csv(&dataset, argv[2], ',', &error);
        if (status == MANO_OK) dataset_print(&dataset, 10, stdout);
        dataset_destroy(&dataset);
        if (status != MANO_OK) { mano_error_print(&error, stderr); return 1; }
        return 0;
    }

    usage(argv[0]);
    return 2;
}
