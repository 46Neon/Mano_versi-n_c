#include "logger.h"
#include <time.h>
#include <stdarg.h>

static const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

const char *milena_log_level_name(MilenaLogLevel level) {
    return level >= MILENA_LOG_DEBUG && level <= MILENA_LOG_FATAL ? level_names[level] : "UNKNOWN";
}

void milena_logger_init(MilenaLogger *logger, MilenaLogLevel minimum_level,
                      FILE *output, bool json_format) {
    if (!logger) return;
    logger->minimum_level = minimum_level;
    logger->output = output ? output : stderr;
    logger->json_format = json_format;
    logger->job_id = NULL;
}

void milena_logger_destroy(MilenaLogger *logger) {
    if (!logger) return;
    free(logger->job_id);
    logger->job_id = NULL;
}

MilenaStatus milena_logger_set_job_id(MilenaLogger *logger, const char *job_id) {
    if (!logger) return MILENA_ERR_ARGUMENT;
    char *copy = job_id ? milena_strdup(job_id) : NULL;
    if (job_id && !copy) return MILENA_ERR_MEMORY;
    free(logger->job_id);
    logger->job_id = copy;
    return MILENA_OK;
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

void milena_logger_log(MilenaLogger *logger, MilenaLogLevel level,
                     const char *file, int line, const char *function,
                     const char *format, ...) {
    if (!logger || level < logger->minimum_level || level > MILENA_LOG_FATAL) return;
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
        fprintf(logger->output, ",\"level\":"); json_string(logger->output, milena_log_level_name(level));
        fprintf(logger->output, ",\"job_id\":"); json_string(logger->output, logger->job_id);
        fprintf(logger->output, ",\"file\":"); json_string(logger->output, file);
        fprintf(logger->output, ",\"line\":%d,\"function\":", line);
        json_string(logger->output, function);
        fputs(",\"message\":", logger->output); json_string(logger->output, message);
        fputs("}\n", logger->output);
    } else {
        fprintf(logger->output, "[%s] %s: %s\n", timestamp,
                milena_log_level_name(level), message);
    }
    (void)fflush(logger->output);
}
