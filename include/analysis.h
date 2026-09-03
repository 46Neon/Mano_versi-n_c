#ifndef MANO_ANALYSIS_H
#define MANO_ANALYSIS_H

#include "dataset.h"

typedef struct AnalysisResult {
    char *name;
    double value;
    struct AnalysisResult *next;
} AnalysisResult;

bool analysis_ventas(const Dataset *dataset,
                    const char *date_column,
                    const char *price_column,
                    const char *quantity_column,
                    const char *output_json);

#endif
