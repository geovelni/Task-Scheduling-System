#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net.h"
#include "protocol.h"
#include "scheduler.h"
#include "db.h"
#include "log.h"

#define MAX_TASKS 100
#define MAX_RUNS  20



int server_start(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);          /* socket TCP */
    if (fd < 0) {
        log_msg(LOG_ERROR, "socket: %s", strerror(errno));
        return -1;
    }


    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;            
    addr.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_msg(LOG_ERROR, "bind pe portul %d: %s", port, strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, 10) < 0) {
        log_msg(LOG_ERROR, "listen: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

/* ---------- Raspunsuri ---------- */

static void send_error(int fd, int code)
{
    const char *names[] = {
        "", "COMANDA_NECUNOSCUTA", "ARGUMENTE_INVALIDE", "TASK_INEXISTENT",
        "EROARE_INTERNA", "TIP_NECUNOSCUT", "DATA_INVALIDA", "DATA_IN_TRECUT",
        "EXPRESIE_CRON_INVALIDA"
    };
    dprintf(fd, "ERR %d %s\n", code, names[code]);
}

/* ---------- Comenzile ---------- */

static void cmd_add(int fd, const char *line)
{
    char type_text[16];
    Task t = {0};

    /* ADD <tip> <nume> <durata_max> "<programare>" <comanda> */
    int n = sscanf(line, "ADD %15s %63s %d \"%63[^\"]\" %255[^\n]",
                   type_text, t.name, &t.max_duration, t.schedule, t.command);
    if (n != 5 || t.max_duration <= 0) {
        log_msg(LOG_WARN, "ADD cu argumente invalide: \"%s\"", line);
        send_error(fd, ERR_INVALID_ARGS);
        return;
    }

    if (strcmp(type_text, "ONCE") == 0)
        t.type = TASK_ONCE;
    else if (strcmp(type_text, "REPEAT") == 0)
        t.type = TASK_REPEAT;
    else {
        log_msg(LOG_WARN, "Tip de task necunoscut: \"%s\"", type_text);
        send_error(fd, ERR_UNKNOWN_TYPE);
        return;
    }

    int err = scheduler_first_run(t.type, t.schedule, &t.next_run);
    if (err != 0) {
        log_msg(LOG_WARN, "Programare invalida \"%s\" (eroare %d)", t.schedule, err);
        send_error(fd, err);
        return;
    }

    t.active = 1;
    if (db_add_task(&t) != 0) {
        send_error(fd, ERR_INTERNAL);
        return;
    }

    char when[32];
    format_time(t.next_run, when, sizeof(when));
    log_msg(LOG_INFO, "Task %d (%s, %s) adaugat, prima rulare: %s",
            t.id, t.name, type_text, when);
    dprintf(fd, "OK %d\n", t.id);
}

static void cmd_edit(int fd, const char *line)
{
    int id, max_duration;
    char schedule[64], command[256];


    int n = sscanf(line, "EDIT %d %d \"%63[^\"]\" %255[^\n]",
                   &id, &max_duration, schedule, command);
    if (n != 4 || max_duration <= 0) {
        log_msg(LOG_WARN, "EDIT cu argumente invalide: \"%s\"", line);
        send_error(fd, ERR_INVALID_ARGS);
        return;
    }

    Task t;
    if (db_get_task(id, &t) != 0) {
        send_error(fd, ERR_TASK_NOT_FOUND);
        return;
    }

    int err = scheduler_first_run(t.type, schedule, &t.next_run);
    if (err != 0) {
        send_error(fd, err);
        return;
    }

    strcpy(t.schedule, schedule);
    strcpy(t.command, command);
    t.max_duration = max_duration;
    t.active = 1;                          /* poate fi reactivat si un ONCE terminat */

    if (db_update_task(&t) != 0) {
        send_error(fd, ERR_INTERNAL);
        return;
    }
    log_msg(LOG_INFO, "Task %d (%s) modificat", t.id, t.name);
    dprintf(fd, "OK\n");
}

static void cmd_del(int fd, const char *line)
{
    int id;
    char extra;

   
    if (sscanf(line, "DEL %d %c", &id, &extra) != 1) {
        send_error(fd, ERR_INVALID_ARGS);
        return;
    }

    int r = db_delete_task(id);
    if (r == 1) {
        send_error(fd, ERR_TASK_NOT_FOUND);
    } else if (r != 0) {
        send_error(fd, ERR_INTERNAL);
    } else {
        log_msg(LOG_INFO, "Task %d sters", id);
        dprintf(fd, "OK\n");
    }
}

static void cmd_list(int fd)
{
    Task list[MAX_TASKS];
    int n = db_get_tasks(list, MAX_TASKS);
    if (n < 0) {
        send_error(fd, ERR_INTERNAL);
        return;
    }

    dprintf(fd, "OK %d\n", n);
    for (int i = 0; i < n; i++) {
        Task *t = &list[i];
        char when[32] = "-";
        if (t->active)
            format_time(t->next_run, when, sizeof(when));

        dprintf(fd, "%d|%s|%s|%s|%s|%d|%d|%s\n",
                t->id, t->type == TASK_ONCE ? "ONCE" : "REPEAT", t->name,
                t->schedule, when, t->active, t->max_duration, t->command);
    }
}

static void cmd_history(int fd, const char *line)
{
    int id;
    char extra;
    if (sscanf(line, "HISTORY %d %c", &id, &extra) != 1) {
        send_error(fd, ERR_INVALID_ARGS);
        return;
    }

    Task t;
    if (db_get_task(id, &t) != 0) {
        send_error(fd, ERR_TASK_NOT_FOUND);
        return;
    }

    Run list[MAX_RUNS];
    int n = db_get_runs(id, list, MAX_RUNS);
    if (n < 0) {
        send_error(fd, ERR_INTERNAL);
        return;
    }

    dprintf(fd, "OK %d\n", n);
    for (int i = 0; i < n; i++) {
        char when[32];
        format_time(list[i].start, when, sizeof(when));
        dprintf(fd, "%d|%s|%d|%d|%s\n", list[i].id, when,
                list[i].duration, list[i].exit_code, list[i].result);
    }
}

static void cmd_help(int fd)
{
    dprintf(fd, "OK 7\n");
    dprintf(fd, "ADD <ONCE|REPEAT> <nume> <durata_max_sec> \"<programare>\" <comanda>\n");
    dprintf(fd, "EDIT <id> <durata_max_sec> \"<programare>\" <comanda>\n");
    dprintf(fd, "DEL <id>\n");
    dprintf(fd, "LIST\n");
    dprintf(fd, "HISTORY <id>\n");
    dprintf(fd, "PING\n");
    dprintf(fd, "HELP\n");
}


static void handle_command(int fd, char *line)
{
    char cmd[16] = "";
    sscanf(line, "%15s", cmd);

    if      (strcmp(cmd, "ADD") == 0)     cmd_add(fd, line);
    else if (strcmp(cmd, "EDIT") == 0)    cmd_edit(fd, line);
    else if (strcmp(cmd, "DEL") == 0)     cmd_del(fd, line);
    else if (strcmp(cmd, "LIST") == 0)    cmd_list(fd);
    else if (strcmp(cmd, "HISTORY") == 0) cmd_history(fd, line);
    else if (strcmp(cmd, "HELP") == 0)    cmd_help(fd);
    else if (strcmp(cmd, "PING") == 0)    dprintf(fd, "OK PONG\n");
    else {
        /* Mesaj nedefinit: raspundem cu eroare si mergem mai departe */
        log_msg(LOG_WARN, "Mesaj necunoscut: \"%s\"", line);
        send_error(fd, ERR_UNKNOWN_COMMAND);
    }
}

/* ---------- Un client ---------- */

void server_handle_client(int listen_fd)
{
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    int fd = accept(listen_fd, (struct sockaddr *)&client, &len);
    if (fd < 0) {
        log_msg(LOG_ERROR, "accept: %s", strerror(errno));
        return;
    }

   
    struct timeval timeout = { 3, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char line[1024];
    int n = read(fd, line, sizeof(line) - 1);

    if (n < 0 && errno == EAGAIN) {
        log_msg(LOG_WARN, "Client %s: nu a trimis nimic in 3 secunde, il deconectez",
                inet_ntoa(client.sin_addr));
    } else if (n < 0) {
        log_msg(LOG_ERROR, "read: %s", strerror(errno));
    } else if (n == 0) {
        log_msg(LOG_WARN, "Client %s s-a deconectat fara mesaj", inet_ntoa(client.sin_addr));
    } else {
        line[n] = '\0';
        line[strcspn(line, "\r\n")] = '\0';          /* taiem Enter-ul de la final */

        log_msg(LOG_INFO, "Client %s: %s", inet_ntoa(client.sin_addr), line);
        handle_command(fd, line);
    }

    close(fd);
}
