#ifndef MILENA_FOREST_H
#define MILENA_FOREST_H
#include "array.h"

typedef struct {
    size_t feature;
    double threshold;
    size_t left;
    size_t right;
    int64_t prediction;
    bool leaf;
} MilenaForestNode;

typedef struct {
    size_t tree_count;
    size_t feature_count;
    size_t max_depth;
    size_t *roots;
    MilenaForestNode *nodes;
    size_t node_count;
} MilenaForestClassifier;

void milena_forest_init(MilenaForestClassifier *forest);
void milena_forest_release(MilenaForestClassifier *forest);
MilenaStatus milena_forest_train(MilenaForestClassifier *forest, const MilenaArray *features,
                                  const MilenaArray *labels, size_t tree_count, MilenaError *error);
MilenaStatus milena_forest_train_depth(MilenaForestClassifier *forest, const MilenaArray *features,
                                       const MilenaArray *labels, size_t tree_count, size_t max_depth,
                                       MilenaError *error);
MilenaStatus milena_forest_predict(const MilenaForestClassifier *forest, const MilenaArray *features,
                                   MilenaArray *predictions, MilenaError *error);
#endif
