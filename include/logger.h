#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR
} LogLevel;

void log_init(const char *ident, int option, int facility);

void log_write(LogLevel level, const char* format, ...);

void log_set_level(LogLevel level);

void log_close();

#endif // LOGGER_H