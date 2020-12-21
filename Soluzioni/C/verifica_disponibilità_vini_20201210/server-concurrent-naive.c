#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define N 256

/* Gestore del segnale SIGCHLD */
void handler(int s)
{
        int status;
        /* gestisco tutti i figli terminati */
        while (waitpid(-1, &status, WNOHANG) > 0);
}

int main(int argc, char** argv)
{
        struct addrinfo hints, *res;
        int err, sd, ns, pid, pid2, on, piped[2];
        char nome[N], annata[N];
        struct sigaction sa;
        // IMPORTANTE: creando l'ack aggiungo \n per consentire al client Java la readLine
        char *ack = "ack\n";

        /* Controllo argomenti */
        if (argc < 2){
                printf("Uso: ./server <porta> \n");
                exit(EXIT_FAILURE);
        }

        sigemptyset(&sa.sa_mask);
        /* uso SA_RESTART per evitare di dover controllare esplicitamente se
         * accept è stata interrotta da un segnale e in tal caso rilanciarla
         * (si veda il paragrafo 21.5 del testo M. Kerrisk, "The Linux
         * Programming Interface") */
        sa.sa_flags   = SA_RESTART;
        sa.sa_handler = handler;

        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags    = AI_PASSIVE;

        if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
                fprintf(stderr, "Errore setup indirizzo bind: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
                perror("Errore in socket");
                exit(EXIT_FAILURE);
        }

        on = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0){
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }

        if (bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
                perror("Errore in bind");
                exit(EXIT_FAILURE);
        }

        freeaddrinfo(res);

        if (listen(sd, SOMAXCONN)<0){
                perror("listen");
                exit(EXIT_FAILURE);
        }

        /* Attendo i client... */
        for (;;){
                printf("Server in ascolto...\n");

                if ((ns = accept(sd, NULL, NULL)) < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                /* Generazione di un figlio */
                if ((pid = fork())<0){
                        perror("fork");
                        exit(EXIT_FAILURE);
                } else if (pid == 0){ /* figlio */

                        /* Chiudo la socket passiva */
                        close(sd);

                        memset(nome, 0, sizeof(nome));
                        if (read(ns, nome, N-1)<0){
                                perror("read nome");
                                exit(EXIT_FAILURE);
                        }

                        if (write(ns, ack, strlen(ack))<0){
                                perror("write ack");
                                exit(EXIT_FAILURE);
                        }

                        memset(annata, 0, sizeof(annata));
                        if (read(ns, annata, N-1)<0){
                                perror("read annata");
                                exit(EXIT_FAILURE);
                        }

                        if (pipe(piped) < 0) {
                                perror("pipe");
                                exit(EXIT_FAILURE);
                        }

                        if ((pid2 = fork())<0){
                                perror("seconda fork");
                                exit(EXIT_FAILURE);
                        } else if (pid2 == 0){

                                // Nipote N1
                                // Chiudo i descrittori che non servono
                                close(piped[0]);
                                close(ns);

                                /* Ridireziono stdout */
                                close(1);
                                dup(piped[1]);
                                close(piped[1]);

                                /* Eseguo la grep mandando i risultati al figlio */
                                execlp("grep", "grep", nome, "test.txt", (char*)NULL);	//"/var/local/magazzino.txt"
                                perror("exec prima grep");
                                exit(EXIT_FAILURE);
                        }

                        /* figlio */

                        /* Chiudo descrittori non necessari */
                        close(piped[1]);

                        /* Ridireziono stdin */
                        close(0);
                        dup(piped[0]);
                        close(piped[0]);

                        /* Ridireziono stdout */
                        close(1);
                        dup(ns);
                        close(ns);

                        execlp("grep", "grep", annata, (char *)NULL);
                        perror("exec seconda grep");
                        exit(EXIT_FAILURE);
                }

                /* padre */
                close(ns);
        }

        return 0;
}
