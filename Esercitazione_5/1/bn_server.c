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
const char dir_files[] = "/home/enrico/dirprova/bollettino_neve/"; /* Directory contenente i file .txt delle località */
char file_path[1024];                                              /* Numero assoluto del file della località richiesta, viene generato dopo */

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

    printf("Server connesso in attesa di richieste...\n");

    /* Gestione delle richieste */
    for (;;)
    {
        ns = accept(sd, NULL, NULL);
        if (ns < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* Generazione del processo figlio per gestire la richiesta */
        pid1 = fork();
        if (pid1 < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0)
        {
            /* Codice processo figlio */
            int status;
            char localita[MAX_REQUEST_SIZE], numero_loc[MAX_REQUEST_SIZE];
            char *ack = "OK\n", *end_request = "FINE\n";
            rxb_t rxb;
            size_t len_localita, len_numero_loc;
            pid_t pid2, pid3;
            pipe_t p1p2;

            /* Disinstallo il gestore del segnale SIGCHLD */
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;
            if (sigaction(SIGCHLD, &sa, NULL) < 0)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            /* chiusura socket passiva */
            close(sd);

            /* Inizializzazione del buffer rxb */
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            /* Gestione richiesta singolo client */
            for (;;)
            {
                /* Installo la pipe */
                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                /* Lettura delle informazioni dal client */
                // Località
                memset(localita, 0, sizeof(localita));
                len_localita = sizeof(localita) - 1;
                if (rxb_readline(&rxb, ns, localita, &len_localita) < 0)
                {
                    rxb_destroy(&rxb);
                    break;
                }

                /* Invio ack al client */
                if (write_all(ns, ack, strlen(ack)) < 0)
                {
                    perror("write all");
                    exit(EXIT_FAILURE);
                }

                // Numero località
                memset(numero_loc, 0, sizeof(numero_loc));
                len_numero_loc = sizeof(numero_loc) - 1;
                if (rxb_readline(&rxb, ns, numero_loc, &len_numero_loc) < 0)
                {
                    rxb_destroy(&rxb);
                    break;
                }

                // Solo a scopo di debugging
                /*printf("%s\n", localita);
                printf("%s\n", numero_loc);*/

                /* Generazione nome file */
                memset(file_path, 0, sizeof(file_path));
                sprintf(file_path, "%s%s.txt", dir_files, localita);

                // Solo a scopo di debugging
                printf("%s\n", file_path);

                /* Generazione nipote (head) */
                pid2 = fork();
                if (pid2 < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    /* Codice processo nipote (head) */
                    /* Chiudo la socket attiva */
                    close(ns);
                    /* Chiudo lato lettura pipe */
                    close(p1p2[0]);

                    /* Redirezione output su pipe per nipote 2 */
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    /* execlp head -numero_localita file_path */
                    execlp("head", "head", "-n", numero_loc, file_path, (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                /* Codice figlio, geneto nipote 2 (sort) */
                pid3 = fork();
                if (pid3 < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid3 == 0)
                {
                    /* Codice processo figlio pid3 */
                    /* Chiusuta della pipe */
                    close(p1p2[1]);

                    /* Redirezione input */
                    close(0);
                    if (dup(p1p2[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    /* Redirezione output */
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    /* execlp sort -numero_localita file_path */
                    execlp("sort", "sort", "-n", (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                /* ===================== PARTE OPZIONALE ======================*/
                /* Calcolo media cm di neve creando 3 ulteriori processi figli.
                 * I primo esegue una head nel file come richiesto nell'esercizio,
                 * il secondo esegue una cut sulla prima colonna per selezionare i
                 * cm di neve nella località, il terzo esegue una strtok con separatore
                 * '\n' per estrapolare dall'output della seconda execlp i cm di neve.
                 * Il token viene convertito in long int con strtol e viene calcolata la madia.
                 * SOLUZIONE ACCETTABILE? TROPPO PESANTE? SI PUO' FARE DI MEGLIO? BOH...
                 * 
                 * PROBLEMA: se il numero di località richiesto è maggiore del numero di località
                 * effettivamente presenti nel file, la media viene comunque calcolata usando quel
                 * numero di località inserito. Risultato: media errata. 
                 * 
                 * POSSIBILE SOLUZIONE: wc -l sul numero di linee del file richiesto 
                 * int denominatore = (num_linee_utente > num_linee_file) ? wc -l : num_linee_utente 
                 * Serve un altro processo figlio in comunicazione col padre 
                 * Quando mi gira lo faccio... :) */
                
                /* FIGLIO */
                pid_t pid4, pid5, pid6;
                pipe_t p4p5, p5p6;

                if (pipe(p4p5) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if (pipe(p5p6) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                pid4 = fork();
                if (pid4 < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid4 == 0)
                {
                    // nipote 4 head
                    close(p4p5[0]);
                    close(p5p6[0]);
                    close(p5p6[1]);
                    close(ns);

                    // redirezione output
                    close(1);
                    if (dup(p4p5[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p4p5[1]);

                    execlp("head", "head", "-n", numero_loc, file_path, (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                if ((pid5 = fork()) < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid5 == 0)
                {
                    // nipote 5 cut -n 1
                    close(p4p5[1]);
                    close(p5p6[0]);
                    close(ns);

                    // redirezione input
                    close(0);
                    if (dup(p4p5[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p4p5[0]);

                    // redirezione output
                    close(1);
                    if (dup(p5p6[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p5p6[1]);

                    execlp("cut", "cut", "-f", "1", "-d", ",", (char *)0);
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }

                if ((pid6 = fork()) < 0)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid6 == 0)
                {
                    /* NIPOTE 6 */
                    int n_loc; // numero località 
                    long somma_cm_neve = 0; // accumulatore 
                    char *token, *ptr;
                    char buffer[4096], res_msg[1024];
                    ssize_t nread, ret;

                    // scrive su risultato il calcolo della media fatto
                    close(p4p5[0]);
                    close(p4p5[1]);
                    close(p5p6[1]);

                    /* Leggo dalla pipe p5p6 */
                    if ((nread = read_all(p5p6[0], buffer, sizeof(buffer)) < 0))
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }

                    // separazione della stringa letta da pipe
                    token = strtok(buffer, "\n");
                    while (token != NULL)
                    {
                        ret = strtol(token, &ptr, 10);
                        somma_cm_neve += ret;
                        // token successivo
                        token = strtok(NULL, "\n");
                    }
                    n_loc = strtol(numero_loc, &ptr, 10);
                    sprintf(res_msg, "\n%s %ld\n\n", "La media dei cm di neve è: ", somma_cm_neve / n_loc);

                    if (write_all(ns, res_msg, strlen(res_msg)) < 0)
                    {
                        perror("write");
                        exit(EXIT_SUCCESS);
                    }

                    exit(EXIT_SUCCESS);
                }

                /* ================ FINE PARTE OPZIONALE ======================*/

                /* Chiusura pipe */
                close(p1p2[0]);
                close(p1p2[1]);
                close(p4p5[0]);
                close(p4p5[1]);
                close(p5p6[1]);
                close(p5p6[0]);

                /* Attendo la terminazione dei nipoti */
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
                waitpid(pid3, &status, 0);
                waitpid(pid4, &status, 0);
                waitpid(pid5, &status, 0);
                waitpid(pid6, &status, 0);

                /* Invio la stringa di fine richiesta */
                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write all");
                    exit(EXIT_FAILURE);
                }
                /* Proseguo con nuova richiesta */
            }
            /* Terminazione del figlio */
            exit(EXIT_SUCCESS);
        }
        /* PADRE */
        close(ns);
    }

    /* Chiusura della socket passiva */
    close(sd);

    return 0;
}
