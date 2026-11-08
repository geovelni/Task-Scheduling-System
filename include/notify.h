#ifndef NOTIFY_H
#define NOTIFY_H

/* Trimite o alerta cand un task esueaza sau dureaza prea mult. */
void notify(int task_id, const char *task_name, const char *reason);

#endif
