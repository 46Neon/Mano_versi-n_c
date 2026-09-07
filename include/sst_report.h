#ifndef MILENA_SST_REPORT_H
#define MILENA_SST_REPORT_H

#include "common.h"
#include "sst_advanced.h"
#include "sst_contingency.h"
#include "sst_histogram.h"
#include "sst_inference.h"
#include "sst_model.h"
#include "sst_rates.h"
#include "sst_stats.h"

MilenaStatus sst_report_write_json(const char *filename,
                                 const SstEventList *events,
                                 const SstStats *severity,
                                 const SstHistogram *histogram,
                                 const SstRateResult *rate,
                                 MilenaError *error);

MilenaStatus sst_report_write_advanced_json(
    const char *filename,
    const SstAdvancedStats *advanced,
    const SstPoissonInterval *poisson,
    const SstRiskMeasure *risk,
    const SstMannWhitneyResult *mann_whitney,
    const SstWilcoxonResult *wilcoxon,
    const SstChiSquareResult *chi_square,
    MilenaError *error
);

#endif
