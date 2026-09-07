#include "forest.h"

static double forest_value(const MilenaArray *a, size_t index) {
    if (a->dtype == MILENA_DTYPE_INT64)
        return (double)((const int64_t *)milena_array_const_data(a))[index];
    return ((const double *)milena_array_const_data(a))[index];
}

void milena_forest_init(MilenaForestClassifier *forest) {
    if (forest) { forest->tree_count = 0; forest->feature_count = 0; forest->trees = NULL; }
}

void milena_forest_release(MilenaForestClassifier *forest) {
    if (forest) { free(forest->trees); milena_forest_init(forest); }
}

MilenaStatus milena_forest_train(MilenaForestClassifier *forest,
                                  const MilenaArray *features,
                                  const MilenaArray *labels,
                                  size_t tree_count,
                                  MilenaError *error) {
    if (!forest || !features || !labels || !features->storage || !labels->storage ||
        features->ndim != 2 || labels->ndim != 1 || features->shape[0] != labels->shape[0] ||
        features->shape[0] == 0 || features->shape[1] == 0 || tree_count == 0 ||
        (features->dtype != MILENA_DTYPE_INT64 && features->dtype != MILENA_DTYPE_FLOAT64) ||
        labels->dtype != MILENA_DTYPE_INT64) {
        milena_error_set(error, MILENA_ERR_ARGUMENT, 0, 0, 0, "Datos inválidos para bosque");
        return MILENA_ERR_ARGUMENT;
    }
    MilenaDecisionStump *trees = calloc(tree_count, sizeof(*trees));
    if (!trees) { milena_error_set(error, MILENA_ERR_MEMORY, 0, 0, 0, "Sin memoria para bosque"); return MILENA_ERR_MEMORY; }
    size_t rows = features->shape[0], cols = features->shape[1];
    for (size_t t = 0; t < tree_count; t++) {
        size_t feature = t % cols;
        double low = forest_value(features, feature), high = low;
        for (size_t r = 1; r < rows; r++) { double v = forest_value(features, r * cols + feature); if (v < low) low = v; if (v > high) high = v; }
        double threshold = (low + high) / 2.0;
        size_t left0 = 0, left1 = 0, right0 = 0, right1 = 0;
        for (size_t r = 0; r < rows; r++) {
            int label = forest_value(labels, r) != 0.0;
            if (forest_value(features, r * cols + feature) <= threshold) { if (label) left1++; else left0++; }
            else { if (label) right1++; else right0++; }
        }
        trees[t] = (MilenaDecisionStump){feature, threshold, left1 >= left0, right1 >= right0};
    }
    milena_forest_release(forest); forest->trees = trees; forest->tree_count = tree_count; forest->feature_count = cols;
    return MILENA_OK;
}

MilenaStatus milena_forest_predict(const MilenaForestClassifier *forest,
                                   const MilenaArray *features,
                                   MilenaArray *predictions,
                                   MilenaError *error) {
    if (!forest || !forest->trees || !features || features->ndim != 2 || features->shape[1] != forest->feature_count) {
        milena_error_set(error, MILENA_ERR_ARGUMENT, 0, 0, 0, "Datos inválidos para predicción"); return MILENA_ERR_ARGUMENT;
    }
    size_t shape[] = {features->shape[0]};
    MilenaStatus status = milena_array_zeros(predictions, MILENA_DTYPE_INT64, 1, shape, error);
    if (status != MILENA_OK) return status;
    int64_t *out = milena_array_data(predictions);
    for (size_t r = 0; r < features->shape[0]; r++) {
        size_t votes = 0;
        for (size_t t = 0; t < forest->tree_count; t++) {
            const MilenaDecisionStump *tree = &forest->trees[t];
            double value = forest_value(features, r * features->shape[1] + tree->feature);
            votes += value <= tree->threshold ? (size_t)tree->left_class : (size_t)tree->right_class;
        }
        out[r] = votes * 2 >= forest->tree_count;
    }
    return MILENA_OK;
}
