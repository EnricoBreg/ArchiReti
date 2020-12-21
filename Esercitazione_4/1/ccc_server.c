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

char file_path[] = "/home/enrico/conto_corrente.txt"; /* Percorso file conto_corrente.txt */

void handler(int signo) {
        int status;
        
        /* Padre, ricevuto segnale SIGCHLD */
        printf("Padre, ricevuto segnale %d (%s)\n", signo, "SIGCHLD");
        while (waitpid(-1, &status, WNOHANG) > 0) {
                continue;
        }
}

int main(int argc, char const *argv[])
{
        int err, pid1, pid2, sd, on;
        int p1p2[2];
        char buff[1024];
        struct addrinfo hints;
        struct addrinfo *result;
        struct sigaction sa;

        if (argc != 2) {
                fprintf(stderr, "Errore. Uso corretto: %s numeroporta\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        /* Installo il gestore del segnale SIGCHLD */
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SIGCHLD;
        sa.sa_handler = handler;

        /* Connessione sulla porta */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        err = getaddrinfo(NULL, argv[1], &hints, &result);
        if (err != 0) {
                fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sd == -1) {
                perror("Errore socket()");
                exit(EXIT_FAILURE);
        }

        /* Disabilo attesa uscita fase TIME_WAIT prima di creazioe socket */
        on = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }

        /* Successo, esco dal ciclo */
        if(bind(sd, result->ai_addr, result->ai_addrlen) != 0) {
                perror("Errore bind");
                exit(EXIT_FAILURE);
        }

        freeaddrinfo(result); /* Libero la memoria allocata per getaddrinfo */

        printf("SERVER CONNESSO IN ATTESA DI RICHIESTE...\n");

        if (listen(sd, SOMAXCONN) < 0) {
                perror("Errore listen");
                exit(EXIT_FAILURE);
        }

        /* Ciclo di richieste infinite */
        for(;;) {
                int ns;

                ns = accept(sd, NULL, NULL);
                if (ns == -1) {
                        /* ignoro l'errore e continuo con la richiesta successiva */
                        perror("Errore accept");
                        continue;
                }

                /* Installo la pipe tra il processi P1 e P2 */
                if (pipe(p1p2) < 0) {
                        perror("Errore pipe() p1p2");
                        exit(EXIT_FAILURE);
                }
                
                /* Genero il processo P1 */
                pid1 = fork();
                if (pid1 < 0) {
                        perror("Errore fork() P1"); /* Errore */
                        exit(EXIT_FAILURE);
                } 
                if (pid1 == 0) { /* Codice processo P1 */
                        int nread;
                        char categoria[1501];

                        close(sd); /* Non serve a P1 */

                        close(p1p2[0]); /* Devo scrivere sulla pipe e non leggerci */

                        /* Leggo il nome dell'opearazione da ricerca */
                        memset(categoria, 0, sizeof(categoria));
                        nread = read(ns, categoria, sizeof(categoria)-1);
                        if(nread < 0) {
                                perror("Errore read P1"); /* Errore in Read*/
                                exit(EXIT_FAILURE);
                        }

                        printf("Processo P1, ricevuta categoria: %s\n", categoria);

                        /* Redirezione output su pipe per processo P2 */
                        close(1);
                        if (dup(p1p2[1]) < 0) { /* Controllo errori */
                                perror("Errore dup P1");
                                exit(EXIT_FAILURE);
                        }
                        close(p1p2[1]);

                        /* Execlp di grep */
                        execlp("grep", "grep", categoria, file_path, (char*)0);
                        perror("Errore in execlp P1"); /* Qui non ci arrivo mai */
                        exit(EXIT_FAILURE);
                } /* Fine codice processo P1*/
                
                /* Codice processo Padre */
                pid2 = fork(); /* Generazione del processo P2 */
                if (pid2 < 0) {
                        perror("Errore fork P2"); /* Errore */
                        exit(EXIT_FAILURE);
                }
                if (pid2 == 0) { /* Codice processo P2 */

                        close(sd); /* Non serve */
                        close(p1p2[1]); /* Non serve */
                        /* Redirezione input da pipe */
                        close(0);
                        if (dup(p1p2[0]) < 0) { /* Controllo errori */
                                perror("Errore dup 1 P2"); 
                                exit(EXIT_FAILURE);
                        }
                        close(p1p2[0]);

                        printf("Processo P2, riordino e invio al client...\n");

                        /* Redirezione output su socket */
                        close(1);
                        if (dup(ns) < 0) {
                                perror("Errore dup 2 P2");
                                exit(EXIT_FAILURE);
                        }
                        close(ns);

                        /* execlp sort */
                        execlp("grep", "grep", "-n",(char*)0);
                        perror("Errore execlp P2"); /* Qui non ci arrivo mai */
                        exit(EXIT_FAILURE);
                } /* Fine codice processo P2 */

                /* Codice processo Padre */
                close(ns); /* Non serve */
                /* Chiusura lati pipe che non servono */
                close(p1p2[0]);
                close(p1p2[1]);

                /* Continuo con la prossima richiesta */
        } /* End for */

        close(sd);

        return 0;
}