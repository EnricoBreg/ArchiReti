#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

typedef enum
{
    STDIN,
    STDOUT,
    STDERR
} OUT_TYPE;

int main(int argc, char const *argv[])
{
    int sd, err;
    struct addrinfo hints, *res;
    char req_buff[1024];

    if (argc != 2)
    {
        fprintf(stderr, "Errore. Uso correto: %s numeroporta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Preparazione delle direttive getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, argv[1], &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    // Creazione della socket
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0)
    {
        perror("errore socket server");
        exit(EXIT_FAILURE);
    }
    // Bind socket
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Errore bind socket server");
        exit(EXIT_FAILURE);
    }

    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("errore listen server");
        exit(EXIT_FAILURE);
    }

    // Ciclo infinito di richieste
    for (;;)
    {
        int ns, pid;

        ns = accept(sd, NULL, NULL);
        if (ns < 0)
        {
            perror("errore accept server");
            exit(EXIT_FAILURE);
        }

        // Creazione del processo figlio per gestire la richiesta
        pid = fork();
        if (pid < 0)
        {
            perror("Errore fork server");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            // CODICE PROCESSO FIGLIO
            int nread;
            char ps_args[1024];

            close(sd);

            // Lettura da socket degli argomenti per il comando ps
            memset(ps_args, 0, sizeof(ps_args));
            nread = read(ns, ps_args, sizeof(ps_args)-1);
            if (nread < 0)
            {
                perror("errore read figlio server");
                exit(EXIT_FAILURE);
            }

            // Redirezione dell'output
            close(STDOUT);
            if (dup(ns) < 0) {
                perror("errore dup");
                exit(EXIT_FAILURE);
            }
            close(ns);
            
            /* Verifico cosa contine la stringa ps_args */
            if (strlen(ps_args) == 0)
            {
                /* In questo caso eseguo la exec senza argomenti */
                execlp("ps", "ps", (char *)0);
                perror("errore execlp 1");
                exit(EXIT_FAILURE);
            }
            else
            {
                /* Altrimenti eseguo la exec con gli argomenti ps_args */
                execlp("ps", "ps", ps_args, (char *)0);
                perror("errore execlp 2");
                exit(EXIT_FAILURE);
            }
        }

        // Codice processo padre
        close(ns);
    }

    // chiusura socket passiva padre
    close(sd);

    return 0;
}
