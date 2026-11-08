#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * Protocolul dintre client si server (text, o comanda pe linie).
 * Clientul se conecteaza, trimite O comanda, primeste raspunsul, apoi
 * serverul inchide conexiunea.
 *
 *   ADD <ONCE|REPEAT> <nume> <durata_max_sec> "<programare>" <comanda>
 *        ex: ADD ONCE backup 60 "2026-11-02 18:00" ./backup.sh
 *        ex: ADD REPEAT raport 30 "30 8 * * *" ./raport.sh   (zilnic la 08:30)
 *   EDIT <id> <durata_max_sec> "<programare>" <comanda>
 *   DEL <id>
 *   LIST
 *   HISTORY <id>
 *   PING
 *   HELP
 *
 * Raspunsuri:
 *   OK ...                        -> comanda a reusit
 *   OK <n> + n linii              -> pentru LIST si HISTORY
 *   ERR <cod> <NUME_EROARE>       -> comanda a esuat
 *
 * Liniile din LIST:    id|tip|nume|programare|urmatoarea_rulare|activ|durata_max|comanda
 * Liniile din HISTORY: id_rulare|start|durata|cod_iesire|rezultat
 */

#define SERVER_PORT 9000

/* Coduri de eroare */
#define ERR_UNKNOWN_COMMAND  1   /* comanda nu exista (mesaj nedefinit) */
#define ERR_INVALID_ARGS     2   /* argumente lipsa sau gresite */
#define ERR_TASK_NOT_FOUND   3   /* nu exista task cu id-ul dat */
#define ERR_INTERNAL         4   /* eroare interna (ex: baza de date) */
#define ERR_UNKNOWN_TYPE     5   /* tipul nu e ONCE sau REPEAT */
#define ERR_INVALID_DATE     6   /* data nu e de forma YYYY-MM-DD HH:MM */
#define ERR_DATE_IN_PAST     7   /* task ONCE programat in trecut */
#define ERR_INVALID_CRON     8   /* expresie cron gresita */

#endif
