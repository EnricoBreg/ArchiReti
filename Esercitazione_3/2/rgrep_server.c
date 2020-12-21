#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include "rxb.h"
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64*1024; // Lunghezza massima richiesta 64 Kbytes

void handler(int signo) {
    int status;

    while(waitpid(-1, &status, WNOHANG) > 0) continue;
}

// Usage: ./rgrep_server porta
int main(int argc, char const *argv[])
{   
    int err, sd, on;
    struct addrinfo hints, *res;
    struct sigaction sa;

    if (argc != 2) {
        fprintf(stderr, "Errore. Uso corretto: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Installo il gestore del segnale SIGCHLD
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("Errore sigaction");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, argv[1], &hints, &res);
    if (err < 0) {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Creo la socket
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0) {
        perror("Errore socket");
        exit(EXIT_FAILURE);
    }
    // Disattivazione tempo di timeout per ricollegare la socket alla porta specificata
    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("Errore setsockopt");
        exit(EXIT_FAILURE);
    }
    // Collego la socket alla porta
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Errore bind");
        exit(EXIT_FAILURE);
    }
    // Deallocazione memoria per getaddrinfo
    freeaddrinfo(res);

    // Trasformo la socket attiva in passiva
    if (listen(sd, SOMAXCONN) < 0) {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }
    
    puts("Server connesso...");

    for(;;) {
        int ns;
        pid_t pid1;

        ns = accept(sd, NULL, NULL);
        if (ns < 0) {
            perror("Errore accept");
            exit(EXIT_FAILURE);
        }

        // Generazione del figlio per accettare la richiesta
        if ((pid1 = fork()) < 0) {
            perror("Errore fork");
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0) {
            int fd, status;
            char nomefile[1024], parola[1024];
            char end_request[] = "\nFINE\n";
            char exists;
            pid_t pid2;
            // Codice processo figlio
            close(sd);

            // Disinstallo il segnale SIGCHLD
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;
            if (sigaction(SIGCHLD, &sa, NULL) < 0) {
                perror("Errore sigaction 2");
                exit(EXIT_FAILURE);
            }

            for(;;) {
                
                memset(nomefile, 0, sizeof(nomefile));
                // Leggo il nome del file
                if (read_all(ns, nomefile, sizeof(nomefile)) < 0) {
                    perror("Errore read nomefile");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", nomefile);

                // Controllo esistenza del file
                if ((fd = open(nomefile, O_RDWR)) > 0) {
                    // Il file esiste
                    exists = 'N';
                }
                else {
                    // Il file non esiste
                    exists = 'S';
                }
                close(fd);

                if (write_all(ns, &exists, sizeof(exists)) < 0) {
                    perror("Errore socket");
                    break;
                }
                
                if (exists == 'S') {
                    // il file esiste
                    
                    // Leggo la parola
                    if (read_all(ns, parola, sizeof(parola)) < 0) {
                        perror("Errore read parola");
                        exit(EXIT_FAILURE);
                    }

                    puts(parola);

                    if ((pid2 = fork()) < 0) {
                        perror("Errore fork 2");
                        exit(EXIT_FAILURE);
                    }
                    else if (pid2 == 0) {
                        // codice processo figlio (grep)

                        // Redirezione output
                        close(1);
                        // controllo errori
                        if (dup(ns) < 0) {
                            perror("Errore dup");
                            exit(EXIT_FAILURE);
                        }
                        close(ns);

                        // execlp
                        execlp("grep", "grep", parola, nomefile, (char*)0);
                        perror("Errore execlpp grep");
                        exit(EXIT_FAILURE);
                    }
                    // attendo la terminazione del figlio (grep)
                    wait(&status);

                    // Invio fine richiesta
                    if (write_all(ns, end_request, sizeof(end_request)) < 0) {
                        perror("Errore write end_request");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            // Terminazione del figlio
            close(ns);
            exit(EXIT_SUCCESS);
        }

        // Padre
        close(ns);
    }
    // Chiudo la socket passiva
    close(sd);
    return 0;
}
