#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "executor.h"
#include "scheduler.h"
#include "db.h"
#include "log.h"
#include "notify.h"

#define MAX_RUNNING 32

typedef struct {
    int  pid;            
    int  task_id;
    char name[64];
    long start;
    int  max_duration;
    int  killed;          
} Running;

static Running running[MAX_RUNNING];


static void run_in_child(Task *t)
{

    setpgid(0, 0);

   
    char path[128];
    snprintf(path, sizeof(path), "output/task_%d.out", t->id);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dup2(fd, 1);       /* stdout -> fisier */
        dup2(fd, 2);       /* stderr -> fisier */
        close(fd);
    }

  
    for (int i = 3; i < 1024; i++)
        close(i);

    char when[32];
    format_time(time(NULL), when, sizeof(when));
    printf("\n===== Rulare %s : %s =====\n", when, t->command);
    fflush(stdout);

   
    execl("/bin/sh", "sh", "-c", t->command, (char *)NULL);

    
    perror("execl");
    _exit(127);
}

void executor_start(Task *t)
{
    int slot = -1;

    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running[i].pid != 0 && running[i].task_id == t->id) {
            log_msg(LOG_WARN, "Task %d (%s) inca ruleaza, sar peste aceasta rulare",
                    t->id, t->name);
            return;
        }
        if (running[i].pid == 0 && slot == -1)
            slot = i;
    }

    if (slot == -1) {
        log_msg(LOG_ERROR, "Prea multe task-uri pornite simultan, task %d amanat", t->id);
        return;
    }

    int pid = fork();

    if (pid < 0) {
        log_msg(LOG_ERROR, "fork a esuat pentru task %d: %s", t->id, strerror(errno));
        notify(t->id, t->name, "nu s-a putut porni procesul (fork)");
        return;
    }

    if (pid == 0)
        run_in_child(t);  


    setpgid(pid, pid);

    
    running[slot].pid          = pid;
    running[slot].task_id      = t->id;
    running[slot].start        = time(NULL);
    running[slot].max_duration = t->max_duration;
    running[slot].killed       = 0;
    snprintf(running[slot].name, sizeof(running[slot].name), "%s", t->name);

    log_msg(LOG_INFO, "Task %d (%s) pornit, PID %d: %s", t->id, t->name, pid, t->command);
}


static void finish(Running *r, int status)
{
    Run run = {0};
    run.task_id  = r->task_id;
    run.start    = r->start;
    run.duration = time(NULL) - r->start;

    if (WIFEXITED(status))
        run.exit_code = WEXITSTATUS(status);        
    else if (WIFSIGNALED(status))
        run.exit_code = 128 + WTERMSIG(status);    

    if (r->killed)
        strcpy(run.result, "TIMEOUT");
    else if (run.exit_code == 0)
        strcpy(run.result, "SUCCESS");
    else
        strcpy(run.result, "FAILED");

    if (db_save_run(&run) != 0)
        log_msg(LOG_ERROR, "Nu pot salva rularea task-ului %d", r->task_id);

    if (strcmp(run.result, "SUCCESS") == 0) {
        log_msg(LOG_INFO, "Task %d (%s) terminat cu succes in %d s",
                r->task_id, r->name, run.duration);
    } else if (strcmp(run.result, "FAILED") == 0) {
        log_msg(LOG_ERROR, "Task %d (%s) a esuat (exit=%d) dupa %d s",
                r->task_id, r->name, run.exit_code, run.duration);

        char reason[64];
        snprintf(reason, sizeof(reason), "a esuat cu codul %d", run.exit_code);
        notify(r->task_id, r->name, reason);
    } else {
        log_msg(LOG_WARN, "Task %d (%s) oprit dupa timeout", r->task_id, r->name);
    }

    r->pid = 0;    /* eliberam locul */
}

void executor_check(void)
{
    long now = time(NULL);

    for (int i = 0; i < MAX_RUNNING; i++) {
        Running *r = &running[i];
        if (r->pid == 0)
            continue;

        int status;
        int result = waitpid(r->pid, &status, WNOHANG);   

        if (result == r->pid) {
            finish(r, status);                             
        } else if (result == 0) {
            /* inca ruleaza: verificam daca a depasit durata maxima */
            if (!r->killed && now - r->start > r->max_duration) {
                log_msg(LOG_ALERT, "Task %d (%s) depaseste durata maxima (%ld s > %d s), il opresc",
                        r->task_id, r->name, now - r->start, r->max_duration);

                kill(-r->pid, SIGKILL);   /* minus = tot grupul de procese */
                r->killed = 1;

                char reason[64];
                snprintf(reason, sizeof(reason), "durata anormala (peste %d s)", r->max_duration);
                notify(r->task_id, r->name, reason);
            }
        } else {
            log_msg(LOG_ERROR, "waitpid a esuat pentru PID %d: %s", r->pid, strerror(errno));
            r->pid = 0;
        }
    }
}
