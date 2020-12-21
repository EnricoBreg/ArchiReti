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

// const unsigned int MAX_REQUEST_SIZE = 64 * 1024;
const char template[] = "/home/enrico/dirprova/macchine_caffe/";
const char n_lines[] = "10";

typedef int pipe_t[2];

int autorizza(const char username[], const char password[]);

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
    pid_t pid_f;

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

    /* Ciclo di gestione delle richieste */
    for (;;)
    {
        /* Accetto o no la connessione... */
        ns = accept(sd, NULL, NULL);
        if (ns < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* Generazione del figlio per gestire la richiesta */
        if ((pid_f = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid_f == 0)
        {
            /* FIGLIO */
            int autorizzazione, status; // 0 o 1
            char username[2048], password[2048];
            char categoria[2000];
            char file_path[2048];
            char *ack = "OK\n", *autor = "SI\n";
            char *end_request = "FINE\n";
            pid_t pid_n1, pid_n2;
            pipe_t pipe_n1n2;
            rxb_t rxb;
            size_t username_len, pass_len, categoria_len;

            /* Disinstallo il gestore segnale per SIGCHLD */
            if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
            {
                perror("signal");
                exit(EXIT_FAILURE);
            }

            /* Chiudo la socket passiva */
            close(sd);

            /* Inizializzazione del buffer rx */
            rxb_init(&rxb, 4096);

            /* Leggo username */
            memset(username, 0, sizeof(username));
            username_len = sizeof(username) - 1;
            if (rxb_readline(&rxb, ns, username, &username_len) < 0)
            {
                rxb_destroy(&rxb);
                break;
            }

            /* Dubugging */
            //puts(username);

            /* Invio ack */
            if (write_all(ns, ack, strlen(ack)) < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }

            /* Leggo password */
            memset(password, 0, sizeof(password));
            pass_len = sizeof(password) - 1;
            if (rxb_readline(&rxb, ns, password, &pass_len) < 0)
            {
                rxb_destroy(&rxb);
                break;
            }

            /* Debugging */
            //puts(password);

            /* Verifico l'autorizzazione utente */
            autorizzazione = autorizza(username, password);
            if (autorizzazione != 1)
            {
                strcpy(autor, "NO");
                if (write_all(ns, autor, strlen(autor)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                /* Dato che non sono autorizzato è inutile proseguire, termino il figlio */
                exit(EXIT_SUCCESS);
            }
            else
            {
                /* Nel caso il client sia autorizzato */
                if (write_all(ns, autor, strlen(autor)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            /* Gestione delle richieste del singolo client */
            for (;;)
            {
                /* Installo la pipe n1n2*/
                if (pipe(pipe_n1n2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                /* Leggo categoria macchina caffe */
                memset(categoria, 0, sizeof(categoria));
                categoria_len = sizeof(categoria) - 1;
                if (rxb_readline(&rxb, ns, categoria, &categoria_len) < 0)
                {
                    /* Il client ha chiuso la connessione */
                    rxb_destroy(&rxb);
                    break;
                }

                /* Percorso file categoria selezionato */
                snprintf(file_path, sizeof(file_path), "%s%s.txt", template, categoria);
                /* Debugging */
                printf("%s\n", file_path);

                /* Genero il figlio per execlp (head) */
                if ((pid_n1 = fork()) < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid_n1 == 0)
                {
                    /* NIPOTE 1 */

                    /* Chiusura descrittori non usati */
                    close(ns);
                    close(pipe_n1n2[0]);

                    /* Redirezione output su pipe n1n2 */
                    close(1);
                    if (dup(pipe_n1n2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[1]);

                    /* execlp head -n 10 */
                    execlp("head", "head", "-n", n_lines, file_path, (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                /* FIGLIO */

                /* Genero il figlio per execlp (cut) */
                if ((pid_n2 = fork()) < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid_n2 == 0)
                {
                    /* NIPOTE 2*/

                    /* Chiusura descrittori non usati */
                    close(pipe_n1n2[1]);

                    /* Redirezione input da pipe n1n2 */
                    close(0);
                    if (dup(pipe_n1n2[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[0]);

                    /* Redirezione output su socket */
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    /* execlp cat -f 1,2,3 -d ',' */
                    execlp("cut", "cut", "-f", "1,3,4", "-d", ",", (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                /* FIGLIO*/

                /* Chiusura descrittori non usati */
                close(pipe_n1n2[0]);
                close(pipe_n1n2[1]);

                /* Attendo la terminazione dei figli */
                waitpid(pid_n1, &status, 0);
                waitpid(pid_n2, &status, 0);

                /* Scrivo FINE per indicare la fine della richiesta in corso */
                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }

                /* Proseguo con una nuova richiesta del client... */
            }

            /* Teminazione del figlio*/
            exit(EXIT_SUCCESS);
        }

        /* PADRE */

        /* Chiudo la socket attiva */
        close(ns);
    }

    /* Chiudo la socket passiva */
    close(sd);

    return 0;
}

int autorizza(const char username[], const char password[])
{
    return 1;
}