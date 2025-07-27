#ifndef UTILS_H
#define UTILS_H

void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_script_error(int line, int column, const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif
