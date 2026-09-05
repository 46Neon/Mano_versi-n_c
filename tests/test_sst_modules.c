#include "sst_advanced.h"
#include "sst_contingency.h"
#include "sst_dates.h"
#include "sst_histogram.h"
#include "sst_inference.h"
#include "sst_model.h"
#include "sst_normality.h"
#include "sst_rates.h"
#include "sst_report.h"
#include "sst_stats.h"

int main(void) {
    ManoError error;
    mano_error_clear(&error);

    ManoDate date;
    if (sst_date_parse("2026-02-28", &date, &error) != MANO_OK || !date.valid) return 1;
    ManoDate invalid;
    if (sst_date_parse("2026-02-31", &invalid, &error) == MANO_OK) return 2;

    SstStats stats;
    sst_stats_init(&stats);
    if (sst_stats_add(&stats, 1.0, &error) != MANO_OK) return 3;
    if (sst_stats_add(&stats, 2.0, &error) != MANO_OK) return 4;
    if (stats.count != 2 || stats.mean != 1.5) return 5;

    SstHistogram histogram;
    if (sst_histogram_init(&histogram, 5, 0.0, 5.0, &error) != MANO_OK) return 6;
    if (sst_histogram_add(&histogram, 2.0, &error) != MANO_OK) return 7;

    SstEventList events;
    sst_event_list_init(&events);
    SstEvent event;
    sst_event_init(&event);
    event.ocurrio_incidente = SST_BINARY_TRUE;
    event.has_horas_exposicion = true;
    event.horas_exposicion = 100.0;
    if (sst_event_list_append(&events, &event, &error) != MANO_OK) return 8;

    SstRateResult rate;
    if (sst_rate_from_events(&events, 200000.0, &rate, &error) != MANO_OK) return 9;
    if (!rate.valid || rate.rate != 2000.0) return 10;

    double values[] = {1.0, 2.0, 3.0, 10.0};
    SstAdvancedStats advanced;
    if (sst_advanced_compute(values, NULL, 4, &advanced, &error) != MANO_OK) return 11;
    if (advanced.count != 4 || advanced.p95 < 3.0) return 12;

    double normal_values[] = {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4};
    SstNormalityResult normality;
    if (sst_normality_test(normal_values, 8, &normality, &error) != MANO_OK) return 13;

    SstPoissonInterval interval;
    if (sst_poisson_rate_interval(2, 100.0, 200000.0, 1.96, &interval, &error) != MANO_OK) return 13;
    if (interval.rate != 4000.0) return 14;

    SstRiskMeasure risk;
    if (sst_risk_ratio_odds_ratio(5, 95, 2, 98, &risk, &error) != MANO_OK) return 15;
    if (!risk.valid) return 16;

    const char *areas[] = {"A", "A", "B", "B"};
    const char *incidents[] = {"1", "0", "1", "0"};
    SstContingency2D table;
    sst_contingency_init(&table);
    if (sst_contingency_build(areas, incidents, 4, &table, &error) != MANO_OK) return 17;
    SstChiSquareResult chi;
    if (sst_contingency_chi_square(&table, &chi, &error) != MANO_OK) return 18;
    sst_contingency_destroy(&table);

    sst_event_destroy(&event);
    sst_event_list_destroy(&events);
    sst_histogram_destroy(&histogram);
    return 0;
}
