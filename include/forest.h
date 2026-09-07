#ifndef MILENA_FOREST_H
#define MILENA_FOREST_H

#include "array.h"

typedef struct {
    size_t feature;
    double threshold;
    int64_t left_class;
    int64_t right_class;
} MilenaDecisionStump;

typedef struct {
    size_t tree_count;
    size_t feature_count;
    MilenaDecisionStump *trees;
} MilenaForestClassifier;

void milena_forest_init(MilenaForestClassifier *forest);
void milena_forest_release(MilenaForestClassifier *forest);
MilenaStatus milena_forest_train(MilenaForestClassifier *forest,
                                  const MilenaArray *features,
                                  const MilenaArray *labels,
                                  size_t tree_count,
                                  MilenaError *error);
MilenaStatus milena_forest_predict(const MilenaForestClassifier *forest,
                                   const MilenaArray *features,
                                   MilenaArray *predictions,
                                   MilenaError *error);

#endif
