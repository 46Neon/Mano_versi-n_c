#ifndef MANO_LOGGER_H
#define MANO_LOGGER_H

#include "common.h"

typedef enum {
    MANO_LOG_DEBUG = 0,
    MANO_LOG_INFO = 1,
    MANO_LOG_WARN = 2,
    MANO_LOG_ERROR = 3,
    MANO_LOG_FATAL = 4
} ManoLogLevel;

typedef struct {
    ManoLogLevel minimum_level;
    FILE *output;
    bool json_format;
    char *job_id;
} ManoLogger;

void mano_logger_init(ManoLogger *logger, ManoLogLevel minimum_level,
                      FILE *output, bool json_format);
void mano_logger_destroy(ManoLogger *logger);
ManoStatus mano_logger_set_job_id(ManoLogger *logger, const char *job_id);
void mano_logger_log(ManoLogger *logger, ManoLogLevel level,
                     const char *file, int line, const char *function,
                     const char *format, ...);
const char *mano_log_level_name(ManoLogLevel level);

#define MANO_LOG_DEBUG(logger, ...) mano_logger_log((logger), MANO_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MANO_LOG_INFO(logger, ...) mano_logger_log((logger), MANO_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MANO_LOG_WARN(logger, ...) mano_logger_log((logger), MANO_LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MANO_LOG_ERROR(logger, ...) mano_logger_log((logger), MANO_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif
