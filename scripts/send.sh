#!/bin/sh
# Trimite o comanda serverului, pana e gata clientul colegului.
# Exemplu: ./scripts/send.sh 'LIST'
echo "$1" | nc localhost 9000
