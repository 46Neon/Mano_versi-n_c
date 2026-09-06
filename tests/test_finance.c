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

    return 0;
}
