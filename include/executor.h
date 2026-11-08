#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "task.h"

/* Porneste comanda unui task intr-un proces copil (nu asteapta dupa el). */
void executor_start(Task *t);

/* Verifica procesele pornite: care s-au terminat (salveaza rezultatul)
 * si care au depasit durata maxima (le opreste si trimite alerta). */
void executor_check(void);

#endif
