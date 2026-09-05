#include "sst_dates.h"

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static unsigned days_in_month(int year, unsigned month) {
    static const unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap_year(year)) return 29;
    return month >= 1 && month <= 12 ? days[month - 1] : 0;
}

ManoStatus sst_date_parse(const char *text, ManoDate *date, ManoError *error) {
    if (!text || !date) return MANO_ERR_ARGUMENT;
    date->year = 0;
    date->month = date->day = 0;
    date->valid = false;
    int year;
    unsigned month, day;
    char extra;
    if (sscanf(text, "%d-%u-%u%c", &year, &month, &day, &extra) != 3 ||
        year < 1 || month < 1 || month > 12 || day < 1 ||
        day > days_in_month(year, month)) {
        mano_error_set(error, MANO_ERR_DATA, 0, 0, 0, "Fecha SST inválida; use YYYY-MM-DD");
        return MANO_ERR_DATA;
    }
    date->year = year;
    date->month = month;
    date->day = day;
    date->valid = true;
    return MANO_OK;
}

int sst_date_compare(const ManoDate *left, const ManoDate *right) {
    if (!left || !right || !left->valid || !right->valid) return 0;
    if (left->year != right->year) return left->year < right->year ? -1 : 1;
    if (left->month != right->month) return left->month < right->month ? -1 : 1;
    if (left->day != right->day) return left->day < right->day ? -1 : 1;
    return 0;
}

bool sst_date_is_future(const ManoDate *date, const ManoDate *reference) {
    return sst_date_compare(date, reference) > 0;
}

int64_t sst_date_epoch_days(const ManoDate *date) {
    if (!date || !date->valid) return -1;
    int64_t y = date->year;
    int64_t m = (int64_t)date->month;
    y -= m <= 2;
    int64_t era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned mp = (unsigned)(m + (m > 2 ? -3 : 9));
    unsigned doy = (153 * mp + 2) / 5 + date->day - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}
