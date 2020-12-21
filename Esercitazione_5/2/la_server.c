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
/* Percorso assoluto file articoli da revisionare */
const char file_path[] = "/home/enrico/dirprova/articoli_revisionare.txt";

typedef int pipe_t[2];

/* Dichiarazione delle funzioni */
void handler(int signo);
int autorizza(const char email[], const char pass[]);

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

        printf("Server connesso in attesa di richieste...\n");

        /* Ciclo di accettazione delle richieste */
        for (;;)
        {
                ns = accept(sd, NULL, NULL);
                if (ns < 0)
                {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                /* Generazione del figlio per gestire le richieste di un client */
                if ((pid_f = fork()) < 0)
                {
                        perror("fork");
                        exit(EXIT_FAILURE);
                }
                else if (pid_f == 0)
                {
                        /* FIGLIO */
                        int aut, status; /* Autorizzazione */
                        char email[2048], password[2048];
                        char nome_rivista[MAX_REQUEST_SIZE];
                        char *ack = "OK\n", *nack = "NO\n", *end_request = "FINE\n";
                        rxb_t rxb;
                        size_t email_len, pass_len, nomeriv_len, ack_len, end_len;
                        pipe_t pipe_n1n2, pipe_n2n3;
                        pid_t pid_n1, /* grep email */
                            pid_n2,   /* grep nome rivista */
                            pid_n3;   /* sort */

                        /* Chiudo socket passiva */
                        close(sd);

                        /* Disinstallo il gestore del segnale */
                        signal(SIGCHLD, SIG_DFL);

                        /* Inizializzazione del buffer */
                        rxb_init(&rxb, MAX_REQUEST_SIZE);

                        /* Leggo email da server */
                        memset(email, 0, sizeof(email));
                        email_len = sizeof(email) - 1;
                        if (rxb_readline(&rxb, ns, email, &email_len) < 0)
                        {
                                /* Il client ha chiuso la connesione */
                                rxb_destroy(&rxb);
                                exit(EXIT_FAILURE);
                        }

                        /* debugging */
                        //printf("%s", email);

                        /* Invio ack al client */
                        ack_len = strlen(ack);
                        if (write_all(ns, ack, ack_len) < 0)
                        {
                                perror("write");
                                exit(EXIT_FAILURE);
                        }

                        /* Leggo password da server */
                        memset(password, 0, sizeof(password));
                        pass_len = sizeof(password) - 1;
                        if (rxb_readline(&rxb, ns, password, &pass_len) < 0)
                        {
                                /* Il client ha chiuso la connesione */
                                rxb_destroy(&rxb);
                                exit(EXIT_FAILURE);
                        }

                        /* debugging */
                        //printf("%s", password);

                        /* Verifico autorizzazione */
                        aut = autorizza(email, password);
                        /* Invio ack o nack al client */
                        if (aut != 1)
                        {
                                /* NACK */
                                if (write_all(ns, nack, ack_len) < 0)
                                {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                }
                                /* Non ha senso continuare, termino */
                                exit(EXIT_SUCCESS);
                        }
                        else
                        {
                                /* ACK */
                                if (write_all(ns, ack, ack_len) < 0)
                                {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                }
                        }

                        /* Il client è autorizzato 
                         * gestisco le richieste client */
                        for (;;)
                        {
                                /* Installo le pipe n1n2 n2n3 */
                                if ((pipe(pipe_n1n2) < 0) || (pipe(pipe_n2n3) < 0))
                                {
                                        perror("pipe");
                                        exit(EXIT_FAILURE);
                                }

                                /* Leggo il nome della rivista */
                                memset(nome_rivista, 0, sizeof(nome_rivista));
                                nomeriv_len = sizeof(nome_rivista);
                                if (rxb_readline(&rxb, ns, nome_rivista, &nomeriv_len) < 0)
                                {
                                        /* Il client ha chiuso la connesione */
                                        rxb_destroy(&rxb);
                                        break;
                                }

                                /* Genero il nipote 1 (grep) */
                                if ((pid_n1 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n1 == 0)
                                {
                                        /* NIPOTE 1 */

                                        /* Chiudo i descrittori non usati */
                                        close(ns);
                                        close(pipe_n1n2[0]);
                                        close(pipe_n2n3[0]);
                                        close(pipe_n2n3[1]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(pipe_n1n2[1]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n1n2[1]);

                                        /* execlp */
                                        execlp("grep", "grep", email, file_path, (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */

                                /* Genero il nipote 2 (grep) */
                                if ((pid_n2 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n2 == 0)
                                {
                                        /* NIPOTE 2*/

                                        /* Chiudo i descrittori non usati */
                                        close(ns);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);

                                        /* Redirezione dell'input */
                                        close(0);
                                        if (dup(pipe_n1n2[0]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n1n2[0]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(pipe_n2n3[1]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n2n3[1]);

                                        /* execlp */
                                        execlp("grep", "grep", nome_rivista, (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */

                                /* Genero il nipote 2 (sort) */
                                if ((pid_n3 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n3 == 0)
                                {
                                        /* NIPOTE 2*/

                                        /* Chiudo i descrittori non usati */
                                        close(pipe_n1n2[0]);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[1]);

                                        /* Redirezione dell'input */
                                        close(0);
                                        if (dup(pipe_n2n3[0]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n2n3[0]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(ns) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(ns);

                                        /* execlp */
                                        execlp("sort", "sort", "-n", (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */
                                /* ======================== PARTE OPZIONALE ============================ */
                                pid_t pid_n4, pid_n5, pid_n6, pid_n7;
                                pipe_t pipe_n4n5, pipe_n5n6, pipe_n6n7;

                                /* Installo le pipe n4n5 n5n6 n6n7 */
                                if ((pipe(pipe_n4n5) < 0) || (pipe(pipe_n5n6) < 0) || (pipe(pipe_n6n7) < 0))
                                {
                                        perror("pipe");
                                        exit(EXIT_FAILURE);
                                }

                                /* Genero il nipote 4 (grep) */
                                if ((pid_n4 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n4 == 0)
                                {
                                        /* NIPOTE 4 */

                                        /* Chiudo i descrittori non usati */
                                        close(ns);
                                        close(pipe_n1n2[0]);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);
                                        close(pipe_n2n3[1]);
                                        close(pipe_n4n5[0]);
                                        close(pipe_n5n6[0]);
                                        close(pipe_n5n6[1]);
                                        close(pipe_n6n7[0]);
                                        close(pipe_n6n7[1]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(pipe_n4n5[1]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n4n5[1]);

                                        /* execlp */
                                        execlp("grep", "grep", email, file_path, (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */

                                /* Genero il nipote 5 (grep) */
                                if ((pid_n5 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n5 == 0)
                                {
                                        /* NIPOTE 5 */

                                        /* Chiudo i descrittori non usati */
                                        close(ns);
                                        close(pipe_n1n2[0]);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);
                                        close(pipe_n2n3[1]);
                                        close(pipe_n4n5[1]);
                                        close(pipe_n5n6[0]);
                                        close(pipe_n6n7[0]);
                                        close(pipe_n6n7[1]);

                                        /* Redirezione dell'input */
                                        close(0);
                                        if (dup(pipe_n4n5[0]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n4n5[0]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(pipe_n5n6[1]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n5n6[1]);

                                        /* execlp */
                                        execlp("grep", "grep", nome_rivista, (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */

                                /* Genero il nipote 6 (wc) */
                                if ((pid_n6 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n6 == 0)
                                {
                                        /* NIPOTE 6 */

                                        /* Chiudo i descrittori non usati */
                                        close(pipe_n1n2[0]);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);
                                        close(pipe_n2n3[1]);
                                        close(pipe_n4n5[0]);
                                        close(pipe_n4n5[1]);
                                        close(pipe_n5n6[1]);
                                        close(pipe_n6n7[0]);

                                        /* Redirezione dell'input */
                                        close(0);
                                        if (dup(pipe_n5n6[0]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n5n6[0]);

                                        /* Redirezione output */
                                        close(1);
                                        if (dup(pipe_n6n7[1]) < 0)
                                        {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n6n7[1]);

                                        /* execlp */
                                        execlp("wc", "wc", "-l", (char *)0);
                                        perror("execlp");
                                        exit(EXIT_FAILURE);
                                }

                                /* Genero il nipote 7 scrive il risultato */
                                if ((pid_n7 = fork()) < 0)
                                {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                }
                                else if (pid_n7 == 0)
                                {
                                        /* NIPOTE 7 */
                                        int nread;
                                        char n_articoli[50];
                                        char res[1024];

                                        /* Chiudo i descrittori non usati */
                                        close(pipe_n1n2[0]);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);
                                        close(pipe_n2n3[1]);
                                        close(pipe_n4n5[0]);
                                        close(pipe_n4n5[1]);
                                        close(pipe_n5n6[1]);
                                        close(pipe_n6n7[1]);

                                        /* Leggo dalla pipe il risultato della wc -l*/
                                        if ((nread = read_all(pipe_n6n7[0], n_articoli, sizeof(n_articoli))) < 0)
                                        {
                                                perror("read");
                                                exit(EXIT_FAILURE);
                                        }

                                        /* Scrivo il messagio da inviare... */
                                        snprintf(res, sizeof(res), "\nNumero di articoli da revisionare: %s\n", n_articoli);

                                        /* ... e lo invio al client */
                                        if (write_all(ns, res, strlen(res)) < 0)
                                        {
                                                perror("write");
                                                exit(EXIT_FAILURE);
                                        }

                                        /* Termino l'esecuzione */
                                        exit(EXIT_SUCCESS);
                                }

                                /* ======================= FINE PARTE OPZIONALE ======================== */

                                /* FIGLIO */

                                /* Chiudo descrittori non usati */
                                close(pipe_n1n2[0]);
                                close(pipe_n1n2[1]);
                                close(pipe_n2n3[0]);
                                close(pipe_n2n3[1]);
                                /* Descrittori della parte opzionale */
                                close(pipe_n4n5[0]);
                                close(pipe_n4n5[1]);
                                close(pipe_n5n6[0]);
                                close(pipe_n5n6[1]);
                                close(pipe_n6n7[0]);
                                close(pipe_n6n7[1]);

                                /* Attendo la terminazione dei figlio */
                                waitpid(pid_n1, &status, 0);
                                waitpid(pid_n2, &status, 0);
                                waitpid(pid_n3, &status, 0);
                                /* Figlio parte opzionale */
                                waitpid(pid_n4, &status, 0);
                                waitpid(pid_n5, &status, 0);
                                waitpid(pid_n6, &status, 0);
                                waitpid(pid_n7, &status, 0);

                                /* Invio end request al client */
                                end_len = strlen(end_request);
                                if (write_all(ns, end_request, end_len) < 0)
                                {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                }

                                /* Proseguo con una nuova richiesta... */
                        }

                        /* Terminazione del figlio */
                        exit(EXIT_SUCCESS);
                }

                /* PADRE */

                /* Chiudo la socket attiva */
                close(ns);

                /* Proseguo con nuova accettazione... */
        }

        /* Chiudo la socket passiva */
        close(sd);

        /* Termino */
        return 0;
}

/* Definizione delle funzioni */
void handler(int signo)
{
        int status;

        /* Padre, ricevuto segnale SIGCHLD */
        while (waitpid(-1, &status, WNOHANG) > 0)
        {
                continue;
        }
}

int autorizza(const char email[], const char pass[])
{
        return 1;
}