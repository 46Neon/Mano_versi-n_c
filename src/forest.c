#include "forest.h"

static double forest_value(const MilenaArray *a, size_t index) {
    if (a->dtype == MILENA_DTYPE_INT64)
        return (double)((const int64_t *)milena_array_const_data(a))[index];
    return ((const double *)milena_array_const_data(a))[index];
}

static int64_t forest_majority(const MilenaArray *labels, const MilenaArray *features,
                               size_t feature, double threshold, bool left) {
    size_t rows = features->shape[0];
    const int64_t *values = milena_array_const_data(labels);
    int64_t best = values[0]; size_t best_count = 0;
    for (size_t i = 0; i < rows; i++) {
        double value = forest_value(features, i * features->shape[1] + feature);
        if ((value <= threshold) != left) continue;
        size_t count = 0;
        for (size_t j = 0; j < rows; j++) {
            double other = forest_value(features, j * features->shape[1] + feature);
            if ((other <= threshold) == left && values[j] == values[i]) count++;
        }
        if (count > best_count) { best_count = count; best = values[i]; }
    }
    return best;
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
        int64_t left_class = forest_majority(labels, features, feature, threshold, true);
        int64_t right_class = forest_majority(labels, features, feature, threshold, false);
        trees[t] = (MilenaDecisionStump){feature, threshold, left_class, right_class};
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
        int64_t best_label = 0; size_t best_votes = 0;
        for (size_t t = 0; t < forest->tree_count; t++) {
            const MilenaDecisionStump *tree = &forest->trees[t];
            double value = forest_value(features, r * features->shape[1] + tree->feature);
            int64_t label = value <= tree->threshold ? tree->left_class : tree->right_class;
            size_t votes = 0;
            for (size_t u = 0; u <= t; u++) {
                const MilenaDecisionStump *other = &forest->trees[u];
                double other_value = forest_value(features, r * features->shape[1] + other->feature);
                int64_t other_label = other_value <= other->threshold ? other->left_class : other->right_class;
                if (other_label == label) votes++;
            }
            if (votes > best_votes) { best_votes = votes; best_label = label; }
        }
        out[r] = best_label;
    }
    return MILENA_OK;
}
