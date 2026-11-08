# Serverul (`schedulerd`) — partea lui Geo

## Ce face fiecare fișier

| Fișier | Rol |
|---|---|
| `src/server/main.c` | Pornește serverul și rulează bucla principală (`select` cu pauză de 1 s) |
| `src/server/net.c` | Socket TCP, citirea comenzilor, răspunsuri `OK` / `ERR`, mesaje nedefinite |
| `src/server/log.c` | Afișare colorată cu dată, oră și nivel (INFO, WARN, ERROR, ALERT) + `server.log` |
| `src/scheduler/scheduler.c` | Găsește task-urile scadente, le pornește, calculează următoarea rulare |
| `src/scheduler/cron_parse.c` | Verifică și interpretează expresiile cron |
| `src/executor/executor.c` | `fork` + `exec`, `waitpid`, cod de ieșire, oprire la depășirea duratei |
| `src/db/db.c`, `src/notify/notify.c` | Versiuni minime pentru partea colegului (funcționează deja) |

## Bucla principală, pe scurt

```
cât timp serverul rulează:
    așteaptă un client maxim 1 secundă (select)
    dacă a venit un client -> citește comanda, răspunde, închide conexiunea
    scheduler_tick()  -> pornește task-urile ajunse la scadență
    executor_check()  -> verifică ce s-a terminat / ce a depășit durata
```

## Compilare și pornire

```bash
sudo apt-get install -y libsqlite3-dev
make
export TZ=Europe/Bucharest   # altfel orele sunt în UTC (Codespaces)
./schedulerd                 # sau ./schedulerd 9001 pentru alt port
```

Se creează automat: `scheduler.db`, `server.log`, `alerts.log` și folderul `output/`
(acolo ajunge ce afișează fiecare task, în `output/task_<id>.out`).

## Testare fără client (în alt terminal)

```bash
./scripts/send.sh 'PING'
./scripts/send.sh 'ADD REPEAT ok 30 "* * * * *" ./scripts/ok.sh'
./scripts/send.sh 'ADD REPEAT fail 30 "* * * * *" ./scripts/fail.sh'
./scripts/send.sh 'ADD REPEAT slow 5 "* * * * *" ./scripts/slow.sh'
./scripts/send.sh 'ADD ONCE single 30 "2026-12-01 10:00" echo salut'
./scripts/send.sh 'LIST'
./scripts/send.sh 'HISTORY 2'
./scripts/send.sh 'FOO 123'          # mesaj nedefinit -> ERR 1
```

După minutul următor: `ok` reușește, `fail` dă eroare și alertă,
`slow` e oprit după 5 secunde (TIMEOUT) și dă alertă.
Oprirea serverului: `Ctrl + C`.
