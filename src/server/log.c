#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"

static FILE *log_file = NULL;

void log_open(const char *path)
{
    log_file = fopen(path, "a");
    if (log_file == NULL)
        perror("Nu pot deschide fisierul de log");
}

void log_close(void)
{
    if (log_file != NULL)
        fclose(log_file);
}

void log_banner(int port)
{
    printf(BOLD "╔══════════════════════════════════════╗\n");
    printf("║      TASK SCHEDULING SYSTEM          ║\n");
    printf("║      Server pornit pe portul %-5d   ║\n", port);
    printf("╚══════════════════════════════════════╝\n" RESET);
}

void log_msg(int level, const char *format, ...)
{
    const char *names[]  = { "INFO", "WARN", "ERROR", "ALERT" };
    const char *colors[] = { VERDE, GALBEN, ROSU, MOV BOLD };

    /* 1. Construim textul mesajului, ca la printf */
    char text[512];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    /* 2. Data si ora curenta */
    char when[32];
    time_t now = time(NULL);
    strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", localtime(&now));

    /* 3. In terminal: colorat */
    printf(GRI "[%s]" RESET " %s[%-5s]" RESET " %s\n",
           when, colors[level], names[level], text);
    fflush(stdout);

    /* 4. In fisier: fara culori */
    if (log_file != NULL) {
        fprintf(log_file, "[%s] [%-5s] %s\n", when, names[level], text);
        fflush(log_file);
    }
}
