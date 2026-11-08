#ifndef DB_H
#define DB_H

#include "task.h"

/* Toate functiile intorc 0 la succes si -1 la eroare (daca nu scrie altfel). */

int  db_open(const char *path);
void db_close(void);

int db_add_task(Task *t);                    /* completeaza t->id */
int db_update_task(Task *t);
int db_delete_task(int id);                  /* 1 = task inexistent */
int db_get_task(int id, Task *t);            /* -1 = nu exista */

/* Intorc cate task-uri / rulari au fost puse in lista (sau -1) */
int db_get_tasks(Task *list, int max);
int db_get_due_tasks(long now, Task *list, int max);

int db_save_run(Run *r);
int db_get_runs(int task_id, Run *list, int max);

#endif
