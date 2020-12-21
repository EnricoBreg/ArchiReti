#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "rxb.h"
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64 * 1024;

int main(int argc, char const *argv[])
{
        int sd, err;
        struct addrinfo hints, *ptr, *res;
        rxb_t rxb;
        char response[MAX_REQUEST_SIZE];
        char email[2048];    // Regione di interesse
        char password[2048]; // Numero di località da mostrare
        char nome_rivista[MAX_REQUEST_SIZE];
        char ack[3]; // OK o NO
        size_t response_len, email_len, pass_len, nomeriv_len, ack_len;

        /* Controllo argomenti */
        if (argc != 3)
        {
                fprintf(stderr, "Sintassi: %s hostname porta\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        /* Preparo direttive getaddrinfo */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /* Invoco getaddrinfo */
        err = getaddrinfo(argv[1], argv[2], &hints, &res);
        if (err != 0)
        {
                fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        /* Connessione con fallback */
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
        {
                sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                /* Se la socket fallisce, passo all'indirizzo successivo */
                if (sd < 0)
                        continue;

                /* Se la connect va a buon fine, esco dal ciclo */
                if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                        break;

                /* Altrimenti, chiudo la socket e passo all'indirizzo
                 * successivo */
                close(sd);
        }

        /* Controllo che effettivamente il client sia connesso */
        if (ptr == NULL)
        {
                fprintf(stderr, "Errore di connessione!\n");
                exit(EXIT_FAILURE);
        }

        /* A questo punto, posso liberare la memoria allocata da getaddrinfo */
        freeaddrinfo(res);

        /* Inizializzazione del buffer rxb */
        rxb_init(&rxb, MAX_REQUEST_SIZE);

        /* Richiesto email all'utente */
        printf("\nInserire la email ('fine' per uscire):\n");
        if (fgets(email, sizeof(email), stdin) == NULL)
        {
                perror("fgets");
                exit(EXIT_FAILURE);
        }

        /* Controllo se l'utente vuole uscire */
        if (strcmp(email, "fine\n") == 0)
        {
                exit(EXIT_SUCCESS);
        }

        /* Richiesto password all'utente */
        printf("\nInserire la password:\n");
        if (fgets(password, sizeof(password), stdin) == NULL)
        {
                perror("fgets");
                exit(EXIT_FAILURE);
        }

        /* Invio email al server */
        email_len = strlen(email);
        if (write_all(sd, email, email_len) < 0)
        {
                perror("write");
                exit(EXIT_FAILURE);
        }

        /* Ricevo ack */
        memset(ack, 0, sizeof(ack));
        ack_len = sizeof(ack) - 1;
        if (rxb_readline(&rxb, sd, ack, &ack_len) < 0)
        {
                fprintf(stderr, "Errore. Il server ha chiuso la connessione.\n");
                rxb_destroy(&rxb);
                exit(EXIT_FAILURE);
        }

        /* Controllo valore di ack */
        if (ack[0] != 'O' || ack[1] != 'K')
        {
                fprintf(stderr, "Unexpected error occured\n");
                exit(EXIT_FAILURE);
        }

        /* Invio email al server */
        pass_len = strlen(password);
        if (write_all(sd, password, pass_len) < 0)
        {
                perror("write");
                exit(EXIT_FAILURE);
        }

        /* Ricevo l'ack se sono autorizzato o meno */
        memset(ack, 0, sizeof(ack));
        ack_len = sizeof(ack) - 1;
        if (rxb_readline(&rxb, sd, ack, &ack_len) < 0)
        {
                fprintf(stderr, "Errore. Il server ha chiuso la connessione.\n");
                rxb_destroy(&rxb);
                exit(EXIT_FAILURE);
        }

        /* Controllo valore di ack */
        if (ack[0] != 'O' || ack[1] != 'K')
        {
                fprintf(stderr, "Non autorizzato.\n");
                exit(EXIT_SUCCESS);
        }

        /* Se sono qui allora sono autorizzato.
         * proseguo con la gestione richieste utente */
        for (;;)
        {
                memset(nome_rivista, 0, sizeof(nome_rivista));
                /* Richiedo il nome della rivista all'utente */
                printf("\nInserisci il nome della rivista ('fine' per uscire):\n");
                if (fgets(nome_rivista, sizeof(nome_rivista), stdin) == NULL)
                {
                        perror("fgets");
                        exit(EXIT_FAILURE);
                }

                /* Controllo se l'utente vuole uscire */
                if (strcmp(nome_rivista, "fine\n") == 0)
                {
                        break;
                }

                /* Invio nome rivista al server */
                nomeriv_len = strlen(nome_rivista);
                if (write_all(sd, nome_rivista, nomeriv_len) < 0)
                {
                        perror("write");
                        exit(EXIT_FAILURE);
                }

                /* lettura della risposta del server */
                for (;;)
                {
                        memset(response, 0, sizeof(response));
                        response_len = sizeof(response) - 1;
                        /* Leggo il risultato */
                        if (rxb_readline(&rxb, sd, response, &response_len) < 0)
                        {
                                fprintf(stderr, "Errore. Il server ha chiuso la connessione\n");
                                rxb_destroy(&rxb);
                                exit(EXIT_FAILURE);
                        }

                        /* Controllo che il server non abbia smesso di inviare */
                        if (strcmp(response, "FINE") == 0)
                        {
                                break;
                        }

                        /* Stampo la risposta a video */
                        puts(response);
                }
                printf("\n");
        }

        /* Chiudo la socket */
        close(sd);

        /* Termino */
        return 0;
}
