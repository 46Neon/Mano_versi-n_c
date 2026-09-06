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

static uint64_t decimal_abs_u64(int64_t value) {
    return value < 0 ? (uint64_t)(-(value + 1)) + 1u : (uint64_t)value;
}

static bool unsigned_mul_checked(uint64_t left, uint64_t right, uint64_t *out) {
    if (right != 0 && left > UINT64_MAX / right) return false;
    *out = left * right;
    return true;
}

static bool unsigned_pow10(int32_t power, uint64_t *out) {
    uint64_t result = 1;
    for (int32_t i = 0; i < power; i++) {
        if (!unsigned_mul_checked(result, 10u, &result)) return false;
    }
    *out = result;
    return true;
}

static bool signed_from_magnitude(uint64_t magnitude, bool negative,
                                  int64_t *out) {
    uint64_t limit = negative ? (uint64_t)INT64_MAX + 1u : (uint64_t)INT64_MAX;
    if (magnitude > limit) return false;
    if (!negative) {
        *out = (int64_t)magnitude;
    } else if (magnitude == (uint64_t)INT64_MAX + 1u) {
        *out = INT64_MIN;
    } else {
        *out = -(int64_t)magnitude;
    }
    return true;
}

static bool should_round(uint64_t quotient, uint64_t remainder,
                         uint64_t divisor, bool negative,
                         MilenaRoundingMode mode) {
    if (remainder == 0 || mode == MILENA_ROUND_TOWARD_ZERO) return false;
    if (mode == MILENA_ROUND_FLOOR) return negative;
    if (mode == MILENA_ROUND_CEILING) return !negative;
    uint64_t half = divisor / 2u;
    bool above_half = remainder > half;
    bool exact_half = (divisor % 2u == 0u && remainder == half);
    if (mode == MILENA_ROUND_HALF_UP) return above_half || exact_half;
    return above_half || (exact_half && (quotient % 2u != 0u));
}

MilenaStatus milena_decimal_round(MilenaDecimal *out,
                                  const MilenaDecimal *value,
                                  int32_t target_scale,
                                  MilenaRoundingMode mode,
                                  MilenaError *error) {
    if (!out || !decimal_valid(value) || target_scale < 0 ||
        target_scale > MILENA_DECIMAL_MAX_SCALE || mode < MILENA_ROUND_TOWARD_ZERO ||
        mode > MILENA_ROUND_CEILING) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Parámetros de redondeo inválidos");
        return MILENA_ERR_ARGUMENT;
    }
    if (target_scale >= value->scale) {
        int64_t coefficient = 0;
        if (!multiply_power10(value->coefficient, target_scale - value->scale,
                              &coefficient)) {
            finance_error(error, MILENA_ERR_OVERFLOW, "Desbordamiento al ampliar escala");
            return MILENA_ERR_OVERFLOW;
        }
        MilenaDecimal result = {coefficient, target_scale};
        decimal_normalize(&result, error);
        *out = result;
        return MILENA_OK;
    }

    uint64_t divisor = 0;
    if (!unsigned_pow10(value->scale - target_scale, &divisor)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Escala de redondeo fuera de rango");
        return MILENA_ERR_OVERFLOW;
    }
    uint64_t magnitude = decimal_abs_u64(value->coefficient);
    uint64_t quotient = magnitude / divisor;
    uint64_t remainder = magnitude % divisor;
    bool negative = value->coefficient < 0;
    if (should_round(quotient, remainder, divisor, negative, mode)) {
        if (quotient == UINT64_MAX) {
            finance_error(error, MILENA_ERR_OVERFLOW, "Desbordamiento al redondear");
            return MILENA_ERR_OVERFLOW;
        }
        quotient++;
    }
    int64_t coefficient = 0;
    if (!signed_from_magnitude(quotient, negative, &coefficient)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Resultado redondeado fuera del rango int64");
        return MILENA_ERR_OVERFLOW;
    }
    out->coefficient = coefficient;
    out->scale = target_scale;
    decimal_normalize(out, error);
    return MILENA_OK;
}

MilenaStatus milena_decimal_mul(MilenaDecimal *out,
                                const MilenaDecimal *left,
                                const MilenaDecimal *right,
                                MilenaError *error) {
    if (!out || !decimal_valid(left) || !decimal_valid(right)) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Parámetros de multiplicación inválidos");
        return MILENA_ERR_ARGUMENT;
    }
    if (left->scale > MILENA_DECIMAL_MAX_SCALE - right->scale) {
        finance_error(error, MILENA_ERR_OVERFLOW, "La escala del producto supera 18 posiciones");
        return MILENA_ERR_OVERFLOW;
    }
    uint64_t magnitude = 0;
    if (!unsigned_mul_checked(decimal_abs_u64(left->coefficient),
                              decimal_abs_u64(right->coefficient), &magnitude)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Desbordamiento al multiplicar decimales");
        return MILENA_ERR_OVERFLOW;
    }
    int64_t coefficient = 0;
    if (!signed_from_magnitude(magnitude,
                               (left->coefficient < 0) != (right->coefficient < 0),
                               &coefficient)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Producto fuera del rango int64");
        return MILENA_ERR_OVERFLOW;
    }
    out->coefficient = coefficient;
    out->scale = left->scale + right->scale;
    decimal_normalize(out, error);
    return MILENA_OK;
}

MilenaStatus milena_decimal_div(MilenaDecimal *out,
                                const MilenaDecimal *left,
                                const MilenaDecimal *right,
                                int32_t target_scale,
                                MilenaRoundingMode mode,
                                MilenaError *error) {
    if (!out || !decimal_valid(left) || !decimal_valid(right) ||
        right->coefficient == 0 || target_scale < 0 ||
        target_scale > MILENA_DECIMAL_MAX_SCALE) {
        finance_error(error, MILENA_ERR_ARGUMENT, "Parámetros de división inválidos");
        return MILENA_ERR_ARGUMENT;
    }
    int32_t numerator_power = target_scale + right->scale;
    uint64_t numerator = decimal_abs_u64(left->coefficient);
    uint64_t denominator = decimal_abs_u64(right->coefficient);
    uint64_t power = 0;
    if (!unsigned_pow10(numerator_power, &power) ||
        !unsigned_mul_checked(numerator, power, &numerator)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Numerador fuera del rango interno");
        return MILENA_ERR_OVERFLOW;
    }
    if (!unsigned_pow10(left->scale, &power) ||
        !unsigned_mul_checked(denominator, power, &denominator)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Denominador fuera del rango interno");
        return MILENA_ERR_OVERFLOW;
    }
    uint64_t quotient = numerator / denominator;
    uint64_t remainder = numerator % denominator;
    bool negative = (left->coefficient < 0) != (right->coefficient < 0);
    if (should_round(quotient, remainder, denominator, negative, mode)) quotient++;
    if (!signed_from_magnitude(quotient, negative, &out->coefficient)) {
        finance_error(error, MILENA_ERR_OVERFLOW, "Cociente fuera del rango int64");
        return MILENA_ERR_OVERFLOW;
    }
    out->scale = target_scale;
    decimal_normalize(out, error);
    return MILENA_OK;
}
