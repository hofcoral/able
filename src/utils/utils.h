#ifndef UTILS_H
#define UTILS_H

void log_info(const char *fmt, ...);
void log_error_loc(const char *file, int line, const char *fmt, ...);
#define log_error(fmt, ...) log_error_loc(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
void log_debug(const char *fmt, ...);

#endif
