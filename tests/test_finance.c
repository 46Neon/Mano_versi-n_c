#include "finance.h"
#include <assert.h>
#include <string.h>

static void expect_ok(MilenaStatus status, const MilenaError *error) {
    if (status != MILENA_OK) {
        fprintf(stderr, "finance error: %s\n", error ? error->message : "unknown");
        assert(status == MILENA_OK);
    }
}

int main(void) {
    MilenaError error;
    milena_error_clear(&error);

    MilenaDecimal a;
    MilenaDecimal b;
    MilenaDecimal result;
    expect_ok(milena_decimal_from_string(&a, "12.30", &error), &error);
    expect_ok(milena_decimal_from_string(&b, "0.70", &error), &error);
    expect_ok(milena_decimal_add(&result, &a, &b, &error), &error);
    assert(result.coefficient == 13 && result.scale == 0);

    expect_ok(milena_decimal_from_string(&a, "100.00", &error), &error);
    expect_ok(milena_decimal_from_string(&b, "0.125", &error), &error);
    expect_ok(milena_decimal_sub(&result, &a, &b, &error), &error);
    assert(result.coefficient == 99875 && result.scale == 3);

    int comparison = 0;
    expect_ok(milena_decimal_compare(&a, &b, &comparison, &error), &error);
    assert(comparison > 0);

    MilenaDecimal factor;
    MilenaDecimal product;
    expect_ok(milena_decimal_from_string(&factor, "2.5", &error), &error);
    expect_ok(milena_decimal_mul(&product, &b, &factor, &error), &error);
    assert(product.coefficient == 3125 && product.scale == 4);

    MilenaDecimal third;
    expect_ok(milena_decimal_from_i64(&a, 1, &error), &error);
    expect_ok(milena_decimal_from_i64(&b, 3, &error), &error);
    expect_ok(milena_decimal_div(&third, &a, &b, 2, MILENA_ROUND_HALF_UP, &error), &error);
    assert(third.coefficient == 33 && third.scale == 2);

    MilenaDecimal rounded;
    expect_ok(milena_decimal_from_string(&a, "12.345", &error), &error);
    expect_ok(milena_decimal_round(&rounded, &a, 2, MILENA_ROUND_HALF_UP, &error), &error);
    assert(rounded.coefficient == 1235 && rounded.scale == 2);

    MilenaMoney usd_a;
    MilenaMoney usd_b;
    MilenaMoney usd_total;
    expect_ok(milena_money_init(&usd_a, a, "USD", &error), &error);
    expect_ok(milena_money_init(&usd_b, b, "USD", &error), &error);
    expect_ok(milena_money_add(&usd_total, &usd_a, &usd_b, &error), &error);
    assert(strcmp(usd_total.currency, "USD") == 0);

    MilenaMoney eur;
    expect_ok(milena_money_init(&eur, b, "EUR", &error), &error);
    assert(milena_money_add(&usd_total, &usd_a, &eur, &error) == MILENA_ERR_ARGUMENT);

    MilenaRate rate;
    expect_ok(milena_rate_init(&rate, b, MILENA_RATE_PERIODIC, 12, &error), &error);
    assert(rate.periods_per_year == 12);
    assert(rate.kind == MILENA_RATE_PERIODIC);

    MilenaDecimal periodic_rate;
    expect_ok(milena_decimal_from_string(&periodic_rate, "0.01", &error), &error);
    expect_ok(milena_rate_init(&rate, periodic_rate, MILENA_RATE_PERIODIC, 12, &error), &error);
    MilenaDecimal principal;
    MilenaDecimal future;
    expect_ok(milena_decimal_from_i64(&principal, 1000, &error), &error);
    expect_ok(milena_compound_interest(&future, &principal, &rate, 2, &error), &error);
    assert(future.coefficient == 10201 && future.scale == 1);

    MilenaDate start;
    MilenaDate end;
    int64_t days = 0;
    expect_ok(milena_date_init(&start, 2024, 1, 1, &error), &error);
    expect_ok(milena_date_init(&end, 2024, 2, 1, &error), &error);
    expect_ok(milena_date_days_between(&start, &end, &days, &error), &error);
    assert(days == 31);
    MilenaDecimal fraction;
    expect_ok(milena_period_fraction(&start, &end, MILENA_DAY_COUNT_ACTUAL_365,
                                      &fraction, &error), &error);
    assert(fraction.coefficient > 0);

    return 0;
}
