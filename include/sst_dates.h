#ifndef MILENA_SST_DATES_H
#define MILENA_SST_DATES_H

#include "common.h"

typedef struct {
    int year;
    unsigned month;
    unsigned day;
    bool valid;
} MilenaDate;

MilenaStatus sst_date_parse(const char *text, MilenaDate *date, MilenaError *error);
int sst_date_compare(const MilenaDate *left, const MilenaDate *right);
bool sst_date_is_future(const MilenaDate *date, const MilenaDate *reference);
int64_t sst_date_epoch_days(const MilenaDate *date);

#endif
