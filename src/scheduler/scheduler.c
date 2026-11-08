#include <stdio.h>
#include <time.h>
#include "scheduler.h"
#include "executor.h"
#include "db.h"
#include "log.h"
#include "protocol.h"

#define MAX_DUE 50

void format_time(long t, char *buf, int size)
{
    time_t tt = t;
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", localtime(&tt));
}

/* Transforma "2026-11-02 18:00" in secunde. Intoarce -1 daca e gresita. */
static long parse_date(const char *text)
{
    struct tm tm = {0};
    int year, month, day, hour, minute;

    if (sscanf(text, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) != 5)
        return -1;

    tm.tm_year  = year - 1900;
    tm.tm_mon   = month - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_isdst = -1;                       /* lasa sistemul sa decida ora de vara */

    long t = mktime(&tm);

    /* mktime "repara" datele imposibile (ex: 31 februarie -> 3 martie).
     * Daca s-a schimbat ceva, data initiala era gresita. */
    if (t == -1 || tm.tm_mday != day || tm.tm_mon != month - 1 ||
        tm.tm_hour != hour || tm.tm_min != minute)
        return -1;

    return t;
}

int scheduler_first_run(TaskType type, const char *schedule, long *next_run)
{
    long now = time(NULL);

    if (type == TASK_ONCE) {
        long t = parse_date(schedule);
        if (t == -1)
            return ERR_INVALID_DATE;
        if (t <= now)
            return ERR_DATE_IN_PAST;
        *next_run = t;
        return 0;
    }

    /* TASK_REPEAT */
    if (!cron_is_valid(schedule))
        return ERR_INVALID_CRON;
    long t = cron_next(schedule, now);
    if (t == -1)
        return ERR_INVALID_CRON;            /* expresie care nu se potriveste niciodata */
    *next_run = t;
    return 0;
}

/* Se apeleaza din bucla principala, cam o data pe secunda */
void scheduler_tick(void)
{
    Task list[MAX_DUE];
    long now = time(NULL);

    int n = db_get_due_tasks(now, list, MAX_DUE);
    if (n < 0) {
        log_msg(LOG_ERROR, "Nu pot citi task-urile scadente din baza de date");
        return;
    }

    for (int i = 0; i < n; i++) {
        Task *t = &list[i];

        executor_start(t);

        /* Stabilim ce se intampla cu task-ul dupa pornire */
        if (t->type == TASK_ONCE) {
            t->active = 0;                  /* a rulat o data, gata */
        } else {
            t->next_run = cron_next(t->schedule, now);
            if (t->next_run == -1)
                t->active = 0;
        }

        if (db_update_task(t) != 0)
            log_msg(LOG_ERROR, "Nu pot actualiza task-ul %d in baza de date", t->id);
    }
}
