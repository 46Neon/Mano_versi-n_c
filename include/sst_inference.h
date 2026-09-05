#ifndef MANO_SST_INFERENCE_H
#define MANO_SST_INFERENCE_H

#include "common.h"

typedef struct {
    size_t events;
    double exposure;
    double factor;
    double rate;
    double lower;
    double upper;
    bool approximate;
} SstPoissonInterval;

typedef struct {
    size_t exposed_events;
    size_t exposed_non_events;
    size_t control_events;
    size_t control_non_events;
    double risk_exposed;
    double risk_control;
    double relative_risk;
    double odds_ratio;
    double rr_lower;
    double rr_upper;
    double or_lower;
    double or_upper;
    bool continuity_correction;
    bool valid;
} SstRiskMeasure;

typedef struct {
    size_t n_first;
    size_t n_second;
    double u;
    double z;
    double p_value;
    bool approximate;
} SstMannWhitneyResult;

typedef struct {
    size_t pairs;
    double statistic;
    double z;
    double p_value;
    bool approximate;
} SstWilcoxonResult;

ManoStatus sst_poisson_rate_interval(size_t events, double exposure,
                                     double factor, double z_value,
                                     SstPoissonInterval *result,
                                     ManoError *error);
ManoStatus sst_poisson_exact_interval(size_t events, double exposure,
                                      double factor, double confidence_level,
                                      SstPoissonInterval *result,
                                      ManoError *error);
ManoStatus sst_risk_ratio_odds_ratio(size_t exposed_events,
                                      size_t exposed_non_events,
                                      size_t control_events,
                                      size_t control_non_events,
                                      SstRiskMeasure *result,
                                      ManoError *error);
ManoStatus sst_mann_whitney_u(const double *first, size_t n_first,
                              const double *second, size_t n_second,
                              SstMannWhitneyResult *result,
                              ManoError *error);
ManoStatus sst_wilcoxon_signed_rank(const double *before, const double *after,
                                    size_t count, SstWilcoxonResult *result,
                                    ManoError *error);
double sst_chi_square_approx_pvalue(double statistic, size_t degrees_of_freedom);

#endif
