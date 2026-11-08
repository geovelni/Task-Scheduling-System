#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "net.h"
#include "scheduler.h"
#include "executor.h"
#include "db.h"
#include "log.h"
#include "protocol.h"

static volatile int keep_running = 1;


static void stop_server(int sig)
{
    (void)sig;                  
    keep_running = 0;
}

int main(int argc, char *argv[])
{
    int port = SERVER_PORT;
    if (argc > 1)
        port = atoi(argv[1]);

    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    signal(SIGPIPE, SIG_IGN);   

    log_open("server.log");
    mkdir("output", 0755);      

    if (db_open("scheduler.db") != 0) {
        log_msg(LOG_ERROR, "Nu pot deschide baza de date, opresc serverul");
        return 1;
    }

    int listen_fd = server_start(port);
    if (listen_fd < 0) {
        db_close();
        return 1;
    }

    log_banner(port);
    log_msg(LOG_INFO, "Astept clienti si verific task-urile in fiecare secunda");

    while (keep_running) {
    
        fd_set set;
        FD_ZERO(&set);
        FD_SET(listen_fd, &set);
        struct timeval timeout = { 1, 0 };

        int ready = select(listen_fd + 1, &set, NULL, NULL, &timeout);

        if (ready < 0) {
            if (errno == EINTR)         
                continue;
            log_msg(LOG_ERROR, "select: %s", strerror(errno));
            break;
        }

        if (ready > 0)
            server_handle_client(listen_fd);   /* a venit un client */

        scheduler_tick();     /* pornim task-urile scadente */
        executor_check();     /* verificam task-urile care ruleaza */
    }

    log_msg(LOG_INFO, "Server oprit");
    close(listen_fd);
    db_close();
    log_close();
    return 0;
}
