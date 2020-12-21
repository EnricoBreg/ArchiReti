#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "rxb.h"
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64 * 1024;
const char file_path[] = "/home/enrico/dirprova/magazzino.txt"; /* Percorso file magazzino.txt */

typedef int pipe_t[2];

void handler(int signo)
{
    int status;

    /* Padre, ricevuto segnale SIGCHLD */
    while (waitpid(-1, &status, WNOHANG) > 0)
    {
        continue;
    }
}

int main(int argc, char const *argv[])
{
    int err, sd, on, ns;
    struct addrinfo hints;
    struct addrinfo *result;
    struct sigaction sa;
    pid_t pid1;

    if (argc != 2)
    {
        fprintf(stderr, "Errore. Uso corretto: %s numeroporta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Installo il gestore del segnale SIGCHLD */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Connessione sulla porta */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, argv[1], &hints, &result);
    if (err != 0)
    {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sd == -1)
    {
        perror("Errore socket()");
        exit(EXIT_FAILURE);
    }

    /* Disabilo attesa uscita fase TIME_WAIT prima di creazioe socket */
    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Successo, esco dal ciclo */
    if (bind(sd, result->ai_addr, result->ai_addrlen) != 0)
    {
        perror("Errore bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /* Libero la memoriaMAX_REQUEST_SIZE allocata per getaddrinfo */

    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }

    printf("SERVER CONNESSO IN ATTESA DI RICHIESTE...\n");

    /* Gestione richieste */
    for (;;)
    {

        ns = accept(sd, NULL, NULL);
        if (ns < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pid1 = fork();
        if (pid1 < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0)
        {
            /* Codice processo figlio per gestire le richieste */
            int status;
            char nome_vino[MAX_REQUEST_SIZE], annata_vino[MAX_REQUEST_SIZE];
            char *ack = "OK\n", *end_request = "FINE\n";
            size_t len_nome_vino, len_annata_vino;
            rxb_t rxb;
            pid_t pid2, pid3;
            pipe_t p1p2;

            /* Disinstallo il gestore del segnale */
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;

            if (sigaction(SIGCHLD, &sa, NULL) < 0)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            /* Chiusura socket passiva */
            close(sd);

            rxb_init(&rxb, MAX_REQUEST_SIZE);

            /* Gestione richiesta singolo client */
            for (;;)
            {

                /* Installo pipe */
                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                /* leggo le informazioni dal client */
                memset(nome_vino, 0, sizeof(nome_vino));
                len_nome_vino = sizeof(nome_vino) - 1;
                if (rxb_readline(&rxb, ns, nome_vino, &len_nome_vino) < 0)
                {
                    rxb_destroy(&rxb);
                    break;
                }
                // Solo a scopi di debugging
                //printf("%s\n", nome_vino);

                if (write_all(ns, ack, strlen(ack)) < 0)
                {
                    perror("write all");
                    exit(EXIT_FAILURE);
                }

                memset(annata_vino, 0, sizeof(annata_vino));
                len_annata_vino = sizeof(annata_vino) - 1;
                if (rxb_readline(&rxb, ns, annata_vino, &len_annata_vino) < 0)
                {
                    rxb_destroy(&rxb);
                    break;
                }
                // Solo a scopi di debugging
                //printf("%s\n", annata_vino);

                /* Genero nipote (grep) */
                pid2 = fork();
                if (pid2 < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    /* Codice processo nipote (grep) 1 */
                    /* Chiudo la socket */
                    close(ns);
                    /* Chiudo pipe di lettura */
                    close(p1p2[0]);

                    /* redirezione output */
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    /* execlp grep nome_vino */
                    execlp("grep", "grep", nome_vino, file_path, (char *)0);
                    perror("execlp 1");
                    exit(EXIT_FAILURE);
                }

                /* Codice figlio, genero nipote 2 */
                pid3 = fork();
                if (pid3 < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid3 == 0)
                {
                    /* Codice processo nipote (grep) 2 */

                    /* Chiusura lato scrittura pipe */
                    close(p1p2[1]);

                    /* redirezione input */
                    close(0);
                    if (dup(p1p2[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    /* redirezione output */
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    /* execlp grep annata_vino */
                    execlp("grep", "grep", annata_vino, (char *)0);
                    perror("execlp 2");
                    exit(EXIT_FAILURE);
                }

                /* FIGLIO */
                /* Chiusura pipe */
                close(p1p2[0]);
                close(p1p2[1]);

                /* Figlio 1, attendo al terminazione dei figli */
                wait(&status);
                wait(&status);

                /* Invio stringa di fine richiesta */
                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write all");
                    exit(EXIT_FAILURE);
                }
                /* Proseguo con nuova richiesta... */
            }
            /* terminazione del figlio */
            exit(EXIT_SUCCESS);
        }
        /* PADRE */
        close(ns);
    }

    /* Chiudo la socket */
    close(sd);

    return 0;
}
