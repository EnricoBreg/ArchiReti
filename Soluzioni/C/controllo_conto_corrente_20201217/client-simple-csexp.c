#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "rxb.h"
#include "utils.h"
#include "simple_csexp.h"

#define DIM 1024

int main(int argc, char** argv) {
        int err, sd, i = 1;
        struct addrinfo hints, *res, *ptr;
        char *host_remoto, *servizio_remoto;
        char categoria[DIM];
	rxb_t rxb;

        /* Controllo argomenti */
        if (argc < 3) {
                printf("Uso: controllo_conto_corrente <server> <porta>\n");
                exit(EXIT_FAILURE);
        }

        /* Costruzione dell'indirizzo */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        /* Risoluzione dell'host */
        host_remoto = argv[1];
        servizio_remoto = argv[2];

        if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0) {
                fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
                /* se socket fallisce salto direttamente alla prossima iterazione */
                if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
                        fprintf(stderr, "creazione socket fallita\n");
                        continue;
                }
                /* se connect funziona esco dal ciclo */
                if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) {
                        printf("connect riuscita al tentativo %d\n", i);
                        break;
                }
                i++;
                close(sd);
        }

        /* Verifica sul risultato restituito da getaddrinfo */
        if (ptr == NULL) {
                fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato\n");
                exit(EXIT_FAILURE);
        }

        /* Liberiamo la memoria allocata da getaddrinfo() */
        freeaddrinfo(res);

        rxb_init(&rxb, 64 * 1024);

        for(;;) {
                char message[4096];

                /* Lettura dati dall'utente */
                printf("Inserisci la categoria di spese cui sei interessato ('fine' per uscire):\n");
                if (scanf("%s", categoria) == EOF || errno != 0) {
                        perror("scanf");
                        exit(EXIT_FAILURE);
                }

                if (strcmp(categoria, "fine") == 0) {
                        close(sd);
                        printf("Hai scelto di terminare il programma.\n");
                        break;
                }

                snprintf(message, sizeof(message), "(%ld:%s)", strlen(categoria), categoria);

                /* Mando il nome della categoria al server */
                if (write_all(sd, message, strlen(message)) < 0) {
                        perror("write categoria");
                        exit(EXIT_FAILURE);
                }

                /* Ricezione risultato */
                if (simple_csexp_print_message(&rxb, sd) < 0) {
                        fprintf(stderr, "Errore ricezione risposta dal Server\n");
                        exit(EXIT_FAILURE);
                }
        }

        /* ricordarsi sempre di chiudere la socket */
        close(sd);

        return 0;
}
