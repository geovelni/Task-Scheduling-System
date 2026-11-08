/*
 * VERSIUNE MINIMA de pornire (partea colegului).
 * Functioneaza deja; colegul o poate extinde (ex: filtrare istoric, statistici).
 */
#include <stdio.h>
#include <sqlite3.h>
#include "db.h"
#include "log.h"

static sqlite3 *db = NULL;

#define TASK_COLUMNS "id, name, type, schedule, command, next_run, active, max_duration"

int db_open(const char *path)
{
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        log_msg(LOG_ERROR, "SQLite: %s", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name         TEXT NOT NULL,"
        "  type         TEXT NOT NULL CHECK (type IN ('ONCE', 'REPEAT')),"
        "  schedule     TEXT NOT NULL,"
        "  command      TEXT NOT NULL,"
        "  next_run     INTEGER,"
        "  active       INTEGER DEFAULT 1,"
        "  max_duration INTEGER DEFAULT 60);"
        "CREATE TABLE IF NOT EXISTS runs ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  task_id   INTEGER NOT NULL,"
        "  start     INTEGER NOT NULL,"
        "  duration  INTEGER,"
        "  exit_code INTEGER,"
        "  result    TEXT);";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        log_msg(LOG_ERROR, "SQLite: %s", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

void db_close(void)
{
    sqlite3_close(db);
}

/* Pregateste o comanda SQL. Intoarce NULL la eroare. */
static sqlite3_stmt *prepare(const char *sql)
{
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) {
        log_msg(LOG_ERROR, "SQLite: %s", sqlite3_errmsg(db));
        return NULL;
    }
    return st;
}

/* Ruleaza o comanda care nu intoarce randuri (INSERT/UPDATE/DELETE) */
static int run_and_finish(sqlite3_stmt *st)
{
    int ok = sqlite3_step(st) == SQLITE_DONE;
    if (!ok)
        log_msg(LOG_ERROR, "SQLite: %s", sqlite3_errmsg(db));
    sqlite3_finalize(st);
    return ok ? 0 : -1;
}

/* Citeste randurile rezultate intr-o lista de task-uri */
static int read_tasks(sqlite3_stmt *st, Task *list, int max)
{
    int n = 0;
    while (n < max && sqlite3_step(st) == SQLITE_ROW) {
        Task *t = &list[n];
        t->id = sqlite3_column_int(st, 0);
        snprintf(t->name, sizeof(t->name), "%s", sqlite3_column_text(st, 1));
        t->type = (sqlite3_column_text(st, 2)[0] == 'O') ? TASK_ONCE : TASK_REPEAT;
        snprintf(t->schedule, sizeof(t->schedule), "%s", sqlite3_column_text(st, 3));
        snprintf(t->command, sizeof(t->command), "%s", sqlite3_column_text(st, 4));
        t->next_run     = sqlite3_column_int64(st, 5);
        t->active       = sqlite3_column_int(st, 6);
        t->max_duration = sqlite3_column_int(st, 7);
        n++;
    }
    sqlite3_finalize(st);
    return n;
}

int db_add_task(Task *t)
{
    sqlite3_stmt *st = prepare(
        "INSERT INTO tasks (name, type, schedule, command, next_run, active, max_duration)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)");
    if (st == NULL)
        return -1;

    sqlite3_bind_text(st, 1, t->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, t->type == TASK_ONCE ? "ONCE" : "REPEAT", -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 3, t->schedule, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 4, t->command, -1, SQLITE_STATIC);
    sqlite3_bind_int64(st, 5, t->next_run);
    sqlite3_bind_int(st, 6, t->active);
    sqlite3_bind_int(st, 7, t->max_duration);

    if (run_and_finish(st) != 0)
        return -1;
    t->id = sqlite3_last_insert_rowid(db);
    return 0;
}

int db_update_task(Task *t)
{
    sqlite3_stmt *st = prepare(
        "UPDATE tasks SET schedule = ?, command = ?, next_run = ?, active = ?,"
        " max_duration = ? WHERE id = ?");
    if (st == NULL)
        return -1;

    sqlite3_bind_text(st, 1, t->schedule, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, t->command, -1, SQLITE_STATIC);
    sqlite3_bind_int64(st, 3, t->next_run);
    sqlite3_bind_int(st, 4, t->active);
    sqlite3_bind_int(st, 5, t->max_duration);
    sqlite3_bind_int(st, 6, t->id);
    return run_and_finish(st);
}

int db_delete_task(int id)
{
    sqlite3_stmt *st = prepare("DELETE FROM tasks WHERE id = ?");
    if (st == NULL)
        return -1;
    sqlite3_bind_int(st, 1, id);
    if (run_and_finish(st) != 0)
        return -1;
    return sqlite3_changes(db) == 0 ? 1 : 0;     /* 0 randuri sterse = nu exista */
}

int db_get_task(int id, Task *t)
{
    sqlite3_stmt *st = prepare("SELECT " TASK_COLUMNS " FROM tasks WHERE id = ?");
    if (st == NULL)
        return -1;
    sqlite3_bind_int(st, 1, id);
    return read_tasks(st, t, 1) == 1 ? 0 : -1;
}

int db_get_tasks(Task *list, int max)
{
    sqlite3_stmt *st = prepare("SELECT " TASK_COLUMNS " FROM tasks ORDER BY id");
    if (st == NULL)
        return -1;
    return read_tasks(st, list, max);
}

int db_get_due_tasks(long now, Task *list, int max)
{
    sqlite3_stmt *st = prepare(
        "SELECT " TASK_COLUMNS " FROM tasks WHERE active = 1 AND next_run <= ?");
    if (st == NULL)
        return -1;
    sqlite3_bind_int64(st, 1, now);
    return read_tasks(st, list, max);
}

int db_save_run(Run *r)
{
    sqlite3_stmt *st = prepare(
        "INSERT INTO runs (task_id, start, duration, exit_code, result) VALUES (?, ?, ?, ?, ?)");
    if (st == NULL)
        return -1;
    sqlite3_bind_int(st, 1, r->task_id);
    sqlite3_bind_int64(st, 2, r->start);
    sqlite3_bind_int(st, 3, r->duration);
    sqlite3_bind_int(st, 4, r->exit_code);
    sqlite3_bind_text(st, 5, r->result, -1, SQLITE_STATIC);
    return run_and_finish(st);
}

int db_get_runs(int task_id, Run *list, int max)
{
    sqlite3_stmt *st = prepare(
        "SELECT id, task_id, start, duration, exit_code, result FROM runs"
        " WHERE task_id = ? ORDER BY id DESC LIMIT ?");
    if (st == NULL)
        return -1;
    sqlite3_bind_int(st, 1, task_id);
    sqlite3_bind_int(st, 2, max);

    int n = 0;
    while (n < max && sqlite3_step(st) == SQLITE_ROW) {
        list[n].id        = sqlite3_column_int(st, 0);
        list[n].task_id   = sqlite3_column_int(st, 1);
        list[n].start     = sqlite3_column_int64(st, 2);
        list[n].duration  = sqlite3_column_int(st, 3);
        list[n].exit_code = sqlite3_column_int(st, 4);
        snprintf(list[n].result, sizeof(list[n].result), "%s", sqlite3_column_text(st, 5));
        n++;
    }
    sqlite3_finalize(st);
    return n;
}
