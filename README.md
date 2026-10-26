# Task Scheduling System

Sistem de planificare și execuție automată a sarcinilor, similar cu `cron`,
dar cu funcționalități extinse. Permite definirea, programarea și monitorizarea
execuției unor comenzi la momente de timp prestabilite.

## Cerințe

1. **Definirea sarcinilor** – utilizatorul poate crea, edita și șterge task-uri.
2. **Planificator** – un proces central care menține calendarul sarcinilor și
   declanșează execuția la momentul potrivit.
3. **Executor** – rulează efectiv task-urile, local sau distribuit (prin agenți).
4. **Monitorizare și raportare** – salvează istoricul execuțiilor (start, durată,
   rezultat, cod de ieșire) și permite interogarea lui.
5. **Notificări** – alerte (e-mail / log / shell) în caz de eșec sau durată anormală.
6. **Persistență** – task-urile și rezultatele sunt stocate într-o bază de date locală.

## Echipă

| Membru  | Responsabilități                                   |
|---------|----------------------------------------------------|
| Geo Velniceri     | Planificator, Executor, detectarea alertelor       |
| Rareș Ungureanu | Bază de date, interfață CLI, rapoarte, notificări  |

## Tehnologii

- C, Linux (Ubuntu)
- SQLite pentru persistență
- Make pentru compilare

## Compilare și rulare

​```bash
sudo apt install libsqlite3-dev   # o singură dată
make
./schedulerd &                    # pornește planificatorul
./sched add "ls -l" "*/5 * * * *" # adaugă un task
./sched list
​```
