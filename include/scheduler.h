#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

/* Verifica ce task-uri au ajuns la scadenta si le porneste. */
void scheduler_tick(void);

/* Calculeaza prima rulare a unui task nou sau editat.
 * Intoarce 0 daca e in regula, altfel un cod ERR_... din protocol.h */
int scheduler_first_run(TaskType type, const char *schedule, long *next_run);

/* Expresii cron (5 campuri: minut ora zi luna zi_saptamana).
 * Fiecare camp poate fi: *   un numar   sau  * /N (fara spatiu) */
int  cron_is_valid(const char *expr);
long cron_next(const char *expr, long after);

/* Transforma un moment de timp in text "2026-11-02 18:00:00" */
void format_time(long t, char *buf, int size);

#endif
