#ifndef MANO_SST_DATES_H
#define MANO_SST_DATES_H

#include "common.h"

typedef struct {
    int year;
    unsigned month;
    unsigned day;
    bool valid;
} ManoDate;

ManoStatus sst_date_parse(const char *text, ManoDate *date, ManoError *error);
int sst_date_compare(const ManoDate *left, const ManoDate *right);
bool sst_date_is_future(const ManoDate *date, const ManoDate *reference);
int64_t sst_date_epoch_days(const ManoDate *date);

#endif
