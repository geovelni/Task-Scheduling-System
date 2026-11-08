/*
 * VERSIUNE MINIMA de pornire (partea colegului).
 * Acum scrie alertele in alerts.log. Colegul poate adauga e-mail
 * si rularea unui script shell (ex: scripts/alert.sh).
 */
#include <stdio.h>
#include <time.h>
#include "notify.h"

void notify(int task_id, const char *task_name, const char *reason)
{
    FILE *f = fopen("alerts.log", "a");
    if (f == NULL) {
        perror("Nu pot deschide alerts.log");
        return;
    }

    char when[32];
    time_t now = time(NULL);
    strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "[%s] ALERTA task %d (%s): %s\n", when, task_id, task_name, reason);
    fclose(f);
}
