#ifndef MILENA_LOGGER_H
#define MILENA_LOGGER_H

#include "common.h"

typedef enum {
    MILENA_LOG_DEBUG = 0,
    MILENA_LOG_INFO = 1,
    MILENA_LOG_WARN = 2,
    MILENA_LOG_ERROR = 3,
    MILENA_LOG_FATAL = 4
} MilenaLogLevel;

typedef struct {
    MilenaLogLevel minimum_level;
    FILE *output;
    bool json_format;
    char *job_id;
} MilenaLogger;

void milena_logger_init(MilenaLogger *logger, MilenaLogLevel minimum_level,
                      FILE *output, bool json_format);
void milena_logger_destroy(MilenaLogger *logger);
MilenaStatus milena_logger_set_job_id(MilenaLogger *logger, const char *job_id);
void milena_logger_log(MilenaLogger *logger, MilenaLogLevel level,
                     const char *file, int line, const char *function,
                     const char *format, ...);
const char *milena_log_level_name(MilenaLogLevel level);

#define MILENA_LOG_DEBUG(logger, ...) milena_logger_log((logger), MILENA_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MILENA_LOG_INFO(logger, ...) milena_logger_log((logger), MILENA_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MILENA_LOG_WARN(logger, ...) milena_logger_log((logger), MILENA_LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MILENA_LOG_ERROR(logger, ...) milena_logger_log((logger), MILENA_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif
