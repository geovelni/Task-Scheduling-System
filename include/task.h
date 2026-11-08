#ifndef TASK_H
#define TASK_H

/* Tipul unui task */
typedef enum {
    TASK_ONCE,      /* ruleaza o singura data, la o data si ora fixa */
    TASK_REPEAT     /* ruleaza periodic, dupa o expresie cron */
} TaskType;

/* Un task, asa cum e salvat in baza de date */
typedef struct {
    int      id;
    char     name[64];
    TaskType type;
    char     schedule[64];   // "2026-11-02 18:00" (ONCE) sau o expresie cron (REPEAT)
    char     command[256];   /* comanda de rulat, ex: "ls -l" */
    long     next_run;       /* momentul urmatoarei rulari (secunde de la 1970) */
    int      active;         /* 1 = activ, 0 = terminat / dezactivat */
    int      max_duration;   /* durata maxima permisa, in secunde */
} Task;

/* O rulare a unui task (o linie din istoric) */
typedef struct {
    int  id;
    int  task_id;
    long start;              /* cand a pornit */
    int  duration;           /* cat a durat, in secunde */
    int  exit_code;          /* codul de iesire al comenzii */
    char result[16];         /* "SUCCESS", "FAILED" sau "TIMEOUT" */
} Run;

#endif
