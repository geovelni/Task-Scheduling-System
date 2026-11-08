#ifndef LOG_H
#define LOG_H

/* Coduri ANSI pentru culori in terminal */
#define ROSU    "\033[31m"
#define VERDE   "\033[32m"
#define GALBEN  "\033[33m"
#define MOV     "\033[35m"
#define GRI     "\033[90m"
#define BOLD    "\033[1m"
#define RESET   "\033[0m"

/* Nivelurile mesajelor */
#define LOG_INFO   0
#define LOG_WARN   1
#define LOG_ERROR  2
#define LOG_ALERT  3

void log_open(const char *path);
void log_close(void);
void log_banner(int port);

/* Se foloseste exact ca printf: log_msg(LOG_INFO, "Task %d pornit", id); */
void log_msg(int level, const char *format, ...);

#endif
