#include "logger.h"
#include <time.h>
#include <stdarg.h>

static const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

const char *mano_log_level_name(ManoLogLevel level) {
    return level >= MANO_LOG_DEBUG && level <= MANO_LOG_FATAL ? level_names[level] : "UNKNOWN";
}

void mano_logger_init(ManoLogger *logger, ManoLogLevel minimum_level,
                      FILE *output, bool json_format) {
    if (!logger) return;
    logger->minimum_level = minimum_level;
    logger->output = output ? output : stderr;
    logger->json_format = json_format;
    logger->job_id = NULL;
}

void mano_logger_destroy(ManoLogger *logger) {
    if (!logger) return;
    free(logger->job_id);
    logger->job_id = NULL;
}

ManoStatus mano_logger_set_job_id(ManoLogger *logger, const char *job_id) {
    if (!logger) return MANO_ERR_ARGUMENT;
    char *copy = job_id ? mano_strdup(job_id) : NULL;
    if (job_id && !copy) return MANO_ERR_MEMORY;
    free(logger->job_id);
    logger->job_id = copy;
    return MANO_OK;
}

static void json_string(FILE *out, const char *text) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); *p; p++) {
        if (*p == '"') fputs("\\\"", out);
        else if (*p == '\\') fputs("\\\\", out);
        else if (*p == '\n') fputs("\\n", out);
        else if (*p == '\r') fputs("\\r", out);
        else if (*p == '\t') fputs("\\t", out);
        else if (*p < 0x20) fprintf(out, "\\u%04x", *p);
        else fputc(*p, out);
    }
    fputc('"', out);
}

void mano_logger_log(ManoLogger *logger, ManoLogLevel level,
                     const char *file, int line, const char *function,
                     const char *format, ...) {
    if (!logger || level < logger->minimum_level || level > MANO_LOG_FATAL) return;
    char message[1024];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    time_t now = time(NULL);
    struct tm local_time;
    struct tm *time_ptr = localtime(&now);
    if (time_ptr) local_time = *time_ptr;
    else memset(&local_time, 0, sizeof(local_time));
    char timestamp[64] = "unknown";
    (void)strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", &local_time);

    if (logger->json_format) {
        fprintf(logger->output, "{\"timestamp\":"); json_string(logger->output, timestamp);
        fprintf(logger->output, ",\"level\":"); json_string(logger->output, mano_log_level_name(level));
        fprintf(logger->output, ",\"job_id\":"); json_string(logger->output, logger->job_id);
        fprintf(logger->output, ",\"file\":"); json_string(logger->output, file);
        fprintf(logger->output, ",\"line\":%d,\"function\":", line);
        json_string(logger->output, function);
        fputs(",\"message\":", logger->output); json_string(logger->output, message);
        fputs("}\n", logger->output);
    } else {
        fprintf(logger->output, "[%s] %s: %s\n", timestamp,
                mano_log_level_name(level), message);
    }
    (void)fflush(logger->output);
}
