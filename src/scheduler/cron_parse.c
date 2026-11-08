#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "scheduler.h"

/*
 * Expresie cron = 5 campuri:  minut  ora  zi_luna  luna  zi_saptamana
 *   "*"     -> orice valoare
 *   "N"     -> exact valoarea N
 *   "*" + "/N" -> din N in N  (ex: la minut: 0, 5, 10, ...)
 * Exemple: "30 8 * * *"  = in fiecare zi la 08:30
 *          ex. cu "/5" la minut = la fiecare 5 minute
 */

/* Verifica daca un text contine doar cifre */
static int is_number(const char *s)
{
    if (*s == '\0')
        return 0;
    for (int i = 0; s[i] != '\0'; i++)
        if (!isdigit(s[i]))
            return 0;
    return 1;
}

/* Verifica un singur camp (ex: "*", "15", "* /5") si limitele lui */
static int field_is_valid(const char *f, int min, int max)
{
    if (strcmp(f, "*") == 0)
        return 1;

    if (f[0] == '*' && f[1] == '/')               /* forma "* /N" */
        return is_number(f + 2) && atoi(f + 2) > 0;

    if (is_number(f)) {
        int v = atoi(f);
        return v >= min && v <= max;
    }
    return 0;
}

/* Spune daca valoarea "value" se potriveste cu campul "f" */
static int field_matches(const char *f, int value)
{
    if (strcmp(f, "*") == 0)
        return 1;
    if (f[0] == '*' && f[1] == '/')
        return value % atoi(f + 2) == 0;
    return atoi(f) == value;
}

/* Imparte expresia in cele 5 campuri. Intoarce 1 daca sunt exact 5. */
static int split_fields(const char *expr, char f[5][16])
{
    char extra[16];
    int n = sscanf(expr, "%15s %15s %15s %15s %15s %15s",
                   f[0], f[1], f[2], f[3], f[4], extra);
    return n == 5;
}

int cron_is_valid(const char *expr)
{
    char f[5][16];
    if (!split_fields(expr, f))
        return 0;

    return field_is_valid(f[0], 0, 59)     /* minut */
        && field_is_valid(f[1], 0, 23)     /* ora */
        && field_is_valid(f[2], 1, 31)     /* zi din luna */
        && field_is_valid(f[3], 1, 12)     /* luna */
        && field_is_valid(f[4], 0, 6);     /* zi din saptamana (0 = duminica) */
}

/*
 * Gaseste urmatorul minut, dupa momentul "after", care se potriveste.
 * Metoda simpla: incercam minut cu minut, maxim un an.
 * Intoarce -1 daca nu gaseste (ex: "0 0 31 2 *" - 31 februarie).
 */
long cron_next(const char *expr, long after)
{
    char f[5][16];
    if (!split_fields(expr, f))
        return -1;

    time_t t = after - (after % 60) + 60;          /* inceputul minutului urmator */
    int minutes_in_year = 366 * 24 * 60;

    for (int i = 0; i < minutes_in_year; i++) {
        struct tm tm;
        localtime_r(&t, &tm);

        if (field_matches(f[0], tm.tm_min) &&
            field_matches(f[1], tm.tm_hour) &&
            field_matches(f[2], tm.tm_mday) &&
            field_matches(f[3], tm.tm_mon + 1) &&
            field_matches(f[4], tm.tm_wday))
            return t;

        t += 60;
    }
    return -1;
}
