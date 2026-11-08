#ifndef NET_H
#define NET_H

/* Creeaza socketul TCP al serverului. Intoarce descriptorul sau -1. */
int server_start(int port);

/* Accepta un client, citeste comanda, raspunde si inchide conexiunea. */
void server_handle_client(int listen_fd);

#endif
