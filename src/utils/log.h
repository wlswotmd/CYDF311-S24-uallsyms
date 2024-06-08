#ifndef _UALLSYMS_UTILS_LOG_H
#define _UALLSYMS_UTILS_LOG_H

#define LOG_ERROR   0
#define LOG_WARN    1
#define LOG_INFO    2
#define LOG_DEBUG   3

#ifndef LOG_LEVEL
# define LOG_LEVEL LOG_DEBUG
#endif

#ifndef HEXDUMP_COLS
# define HEXDUMP_COLS 16
#endif

#define pr_err(fmt, args...) __pr_log('!', LOG_ERROR, fmt, ## args)
#define pr_warn(fmt, args...) __pr_log('*', LOG_WARN, fmt, ## args)
#define pr_info(fmt, args...) __pr_log('+', LOG_INFO, fmt, ## args)
#define pr_debug(fmt, args...) __pr_log('?', LOG_DEBUG, fmt, ## args)

int __pr_log(char indicator, int log_level, const char *fmt, ...);
void hexdump(void *mem, unsigned int len);

#endif