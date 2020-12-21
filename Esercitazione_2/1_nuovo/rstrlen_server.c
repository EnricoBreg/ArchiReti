#define _POSIX_C_SOURCE 200809L 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <error.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
// Per usare libunistring
#ifdef USE_LIBUNISTRING
    #include <unistr.h>
#endif
#include "rxb.h"
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64*1024; // 64 Kilobytes di richiesta al massimo

/* Gestore del segnale SIGCHLD*/
void handler(int signo) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        continue;
    }
}

int main(int argc, char const *argv[])
{
    int sd, err, on;
    struct addrinfo hints, *res;
    struct sigaction sa;
    // Controllo degli argomenti
    if (argc != 2) {
        fprintf(stderr, "Errore. Uso corretto: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Installo il gestore del segnale
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("Errore sigaction()");
        exit(EXIT_FAILURE);
    }

    // Preparo le direttive per getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    // Invocazione di getaddrinfo
    err = getaddrinfo(NULL, argv[1], &hints, &res);
    if (err < 0) {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    // Creo la socket
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0) {
        perror("Errore in socket()");
        exit(EXIT_FAILURE);
    }
    // Disabilitazione del tempo di attesa per la creazione della socket
    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("Errore setsockopt()");
        exit(EXIT_FAILURE);
    }
    // Connetto il server alla porta
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Errore in bind()");
        exit(EXIT_FAILURE);
    }

    // Libero la memoria allocata a getaddrinfo
    freeaddrinfo(res);
    // Trasformo la socket attiva in passiva
    if (listen(sd, SOMAXCONN) < 0) {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }

    // Ciclo infinito di richieste
    for(;;) {
        int ns, pid;
        
        ns = accept(sd, NULL, NULL);
        if (ns < 0) {
            // Errore in accept, continuo con la prossima richiesta
            perror("Errore accept");
            continue;
        }
        
        // Creazione del processo figlio per gestire la richiesta
        pid = fork();
        if (pid < 0) {
            // Errore in fork
            perror("Errore fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Codice processo figlio
            rxb_t rxb;
#ifdef USE_LIBUNISTRING
            uint8_t request[MAX_REQUEST_SIZE];
#else
            char request[MAX_REQUEST_SIZE];
#endif
            char response[100];
            int string_len;
            size_t request_len;

            // Chiusura della socket passiva
            close(sd);

            // Inizializzazione del buffer di ricezione
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            // Avvio ciclo gestione delle richieste, ogni ciclo riusa la stessa socket
            // Il figlio mantiene la connessione con il client i-esimo
            for(;;) {
                // Inizializzo il buffer a zero e non uso l'ultimo byte
                memset(request, 0, sizeof(request));
                request_len = sizeof(request)-1;

                // Leggo la richiesta dal client
                if (rxb_readline(&rxb, ns, request, &request_len) < 0) {
                    // Se sono qui il client ha chiuso la connessione
                    rxb_destroy(&rxb);
                    break;
                }

                // Conto i caratteri
                string_len = strlen(request);

                printf("Stringa ricevuta: %s\nLunghezza: %d\n", request, string_len);

                // Preparo la risposta
                snprintf(response, sizeof(response), "%d\n", string_len);

                // Invio la risposta al server
                if (write_all(ns, response, strlen(response)) < 0) {
                    perror("Errore write_all");
                    break;
                }
            }
            // Chiusura della socket attiva
            close(ns);
            // Terminazione del figlio
            exit(EXIT_SUCCESS);
        }
        // Codice del processo padre
        // Chiudo la socket attiva
        close(ns);
    }

    close(sd);

    return 0;
}
