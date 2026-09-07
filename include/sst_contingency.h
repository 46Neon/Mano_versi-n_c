#ifndef MILENA_SST_CONTINGENCY_H
#define MILENA_SST_CONTINGENCY_H

#include "common.h"

typedef struct {
    char **row_labels;
    char **column_labels;
    size_t row_count;
    size_t column_count;
    size_t *cells;
} SstContingency2D;

typedef struct {
    double statistic;
    size_t degrees_of_freedom;
    size_t low_expected_cells;
    bool valid;
} SstChiSquareResult;

void sst_contingency_init(SstContingency2D *table);
void sst_contingency_destroy(SstContingency2D *table);
MilenaStatus sst_contingency_build(const char *const *rows,
                                 const char *const *columns,
                                 size_t count,
                                 SstContingency2D *table,
                                 MilenaError *error);
MilenaStatus sst_contingency_chi_square(const SstContingency2D *table,
                                      SstChiSquareResult *result,
                                      MilenaError *error);

#endif
