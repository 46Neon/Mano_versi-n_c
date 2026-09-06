#include "finance.h"

static void finance_error(MilenaError *error, MilenaStatus code,
                          const char *message) {
    if (error) milena_error_set(error, code, 0, 0, 0, message);
}

static bool decimal_valid(const MilenaDecimal *value) {
    return value && value->scale >= 0 && value->scale <= MILENA_DECIMAL_MAX_SCALE;
}

static MilenaStatus decimal_normalize(MilenaDecimal *value, MilenaError *error) {
    if (!decimal_valid(value)) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Decimal inválido");
        return MILENA_ERR_ARGUMENT;
    }
    while (value->scale > 0 && value->coefficient % 10 == 0) {
        value->coefficient /= 10;
        value->scale--;
    }
    return MILENA_OK;
}

static bool multiply_power10(int64_t value, int32_t power, int64_t *out) {
    int64_t result = value;
    for (int32_t i = 0; i < power; i++) {
        if (result > INT64_MAX / 10 || result < INT64_MIN / 10) return false;
        result *= 10;
    }
    *out = result;
    return true;
}

static MilenaStatus decimal_align(const MilenaDecimal *left,
                                  const MilenaDecimal *right,
                                  int64_t *left_coefficient,
                                  int64_t *right_coefficient,
                                  int32_t *scale,
                                  MilenaError *error) {
    if (!decimal_valid(left) || !decimal_valid(right)) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Decimal inválido");
        return MILENA_ERR_ARGUMENT;
    }
    *scale = left->scale > right->scale ? left->scale : right->scale;
    if (!multiply_power10(left->coefficient, *scale - left->scale,
                          left_coefficient) ||
        !multiply_power10(right->coefficient, *scale - right->scale,
                          right_coefficient)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Desbordamiento al alinear decimales");
        return MILENA_ERR_OVERFLOW;
    }
    return MILENA_OK;
}

MilenaStatus milena_decimal_from_i64(MilenaDecimal *out, int64_t value,
                                     MilenaError *error) {
    if (!out) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Salida decimal nula");
        return MILENA_ERR_ARGUMENT;
    }
    out->coefficient = value;
    out->scale = 0;
    return MILENA_OK;
}

MilenaStatus milena_decimal_from_string(MilenaDecimal *out, const char *text,
                                        MilenaError *error) {
    if (!out || !text || text[0] == '\0') {
        finance_error(error, MILENA_ERR_ARGUMENT, "Texto decimal vacío");
        return MILENA_ERR_ARGUMENT;
    }
    size_t position = 0;
    bool negative = false;
    if (text[position] == '+' || text[position] == '-') {
        negative = text[position] == '-';
        position++;
    }
    if (text[position] == '\0') {
        finance_error(error, MILENA_ERR_PARSE, "Decimal sin dígitos");
        return MILENA_ERR_PARSE;
    }

    uint64_t magnitude = 0;
    uint64_t limit = negative ? (uint64_t)INT64_MAX + 1u : (uint64_t)INT64_MAX;
    int32_t scale = 0;
    bool saw_digit = false;
    bool after_decimal = false;
    for (; text[position] != '\0'; position++) {
        char current = text[position];
        if (current == '.') {
            if (after_decimal) {
                finance_error(error, MILENA_ERR_PARSE, "Más de un separador decimal");
                return MILENA_ERR_PARSE;
            }
            after_decimal = true;
            continue;
        }
        if (current < '0' || current > '9') {
            finance_error(error, MILENA_ERR_PARSE, "Decimal inválido: se esperaba un dígito");
            return MILENA_ERR_PARSE;
        }
        saw_digit = true;
        if (after_decimal) {
            if (scale == MILENA_DECIMAL_MAX_SCALE) {
                finance_error(error, MILENA_ERR_OVERFLOW, "El decimal supera 18 posiciones");
                return MILENA_ERR_OVERFLOW;
            }
            scale++;
        }
        uint64_t digit = (uint64_t)(current - '0');
        if (magnitude > (limit - digit) / 10u) {
            finance_error(error, MILENA_ERR_OVERFLOW, "Decimal fuera del rango int64");
            return MILENA_ERR_OVERFLOW;
        }
        magnitude = magnitude * 10u + digit;
    }
    if (!saw_digit) {
        finance_error(error, MILENA_ERR_PARSE, "Decimal sin dígitos");
        return MILENA_ERR_PARSE;
    }
    if (negative) {
        out->coefficient = magnitude == (uint64_t)INT64_MAX + 1u ?
            INT64_MIN : -(int64_t)magnitude;
    } else {
        out->coefficient = (int64_t)magnitude;
    }
    out->scale = scale;
    return decimal_normalize(out, error);
}

MilenaStatus milena_decimal_add(MilenaDecimal *out, const MilenaDecimal *left,
                                const MilenaDecimal *right, MilenaError *error) {
    if (!out) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Salida decimal nula");
        return MILENA_ERR_ARGUMENT;
    }
    int64_t left_value = 0;
    int64_t right_value = 0;
    int32_t scale = 0;
    MilenaStatus status = decimal_align(left, right, &left_value, &right_value,
                                        &scale, error);
    if (status != MILENA_OK) return status;
    if ((right_value > 0 && left_value > INT64_MAX - right_value) ||
        (right_value < 0 && left_value < INT64_MIN - right_value)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Desbordamiento al sumar decimales");
        return MILENA_ERR_OVERFLOW;
    }
    MilenaDecimal result = {left_value + right_value, scale};
    status = decimal_normalize(&result, error);
    if (status == MILENA_OK) *out = result;
    return status;
}

MilenaStatus milena_decimal_sub(MilenaDecimal *out, const MilenaDecimal *left,
                                const MilenaDecimal *right, MilenaError *error) {
    if (!right) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Decimal derecho nulo");
        return MILENA_ERR_ARGUMENT;
    }
    MilenaDecimal negative = *right;
    if (negative.coefficient == INT64_MIN) {
        finance_error(error, MILENA_ERR_OVERFLOW, "No se puede negar el decimal mínimo");
        return MILENA_ERR_OVERFLOW;
    }
    negative.coefficient = -negative.coefficient;
    return milena_decimal_add(out, left, &negative, error);
}

MilenaStatus milena_decimal_compare(const MilenaDecimal *left,
                                    const MilenaDecimal *right, int *result,
                                    MilenaError *error) {
    if (!result) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Resultado de comparación nulo");
        return MILENA_ERR_ARGUMENT;
    }
    int64_t left_value = 0;
    int64_t right_value = 0;
    int32_t scale = 0;
    MilenaStatus status = decimal_align(left, right, &left_value, &right_value,
                                        &scale, error);
    if (status != MILENA_OK) return status;
    *result = left_value < right_value ? -1 : (left_value > right_value ? 1 : 0);
    return MILENA_OK;
}

static MilenaStatus check_currency(const char *currency, MilenaError *error) {
    if (!currency || strlen(currency) != 3 ||
        currency[0] < 'A' || currency[0] > 'Z' ||
        currency[1] < 'A' || currency[1] > 'Z' ||
        currency[2] < 'A' || currency[2] > 'Z') {
        finance_error(error, MILENA_ERR_ARGUMENT, "La moneda debe ser un código ISO de tres letras mayúsculas");
        return MILENA_ERR_ARGUMENT;
    }
    return MILENA_OK;
}

MilenaStatus milena_money_init(MilenaMoney *out, MilenaDecimal amount,
                               const char *currency, MilenaError *error) {
    if (!out || !decimal_valid(&amount)) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Dinero inválido");
        return MILENA_ERR_ARGUMENT;
    }
    MilenaStatus status = check_currency(currency, error);
    if (status != MILENA_OK) return status;
    out->amount = amount;
    memcpy(out->currency, currency, MILENA_CURRENCY_CODE_SIZE);
    return MILENA_OK;
}

static MilenaStatus money_operation(MilenaMoney *out, const MilenaMoney *left,
                                    const MilenaMoney *right, bool subtract,
                                    MilenaError *error) {
    if (!out || !left || !right ||
        strcmp(left->currency, right->currency) != 0) {
        finance_error(error, MILENA_ERR_ARGUMENT, "No se pueden combinar monedas distintas");
        return MILENA_ERR_ARGUMENT;
    }
    MilenaDecimal result = {0};
    MilenaStatus status = subtract ?
        milena_decimal_sub(&result, &left->amount, &right->amount, error) :
        milena_decimal_add(&result, &left->amount, &right->amount, error);
    if (status != MILENA_OK) return status;
    *out = *left;
    out->amount = result;
    return MILENA_OK;
}

MilenaStatus milena_money_add(MilenaMoney *out, const MilenaMoney *left,
                              const MilenaMoney *right, MilenaError *error) {
    return money_operation(out, left, right, false, error);
}

MilenaStatus milena_money_sub(MilenaMoney *out, const MilenaMoney *left,
                              const MilenaMoney *right, MilenaError *error) {
    return money_operation(out, left, right, true, error);
}

MilenaStatus milena_rate_init(MilenaRate *out, MilenaDecimal value,
                              MilenaRateKind kind, uint32_t periods_per_year,
                              MilenaError *error) {
    if (!out || !decimal_valid(&value) || kind < MILENA_RATE_NOMINAL ||
        kind > MILENA_RATE_PERIODIC || periods_per_year == 0) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Tasa o período inválido");
        return MILENA_ERR_ARGUMENT;
    }
    out->value = value;
    out->kind = kind;
    out->periods_per_year = periods_per_year;
    return MILENA_OK;
}
