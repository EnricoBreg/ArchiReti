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

int main(int argc, char *argv[])
{
    int sd, err;
    struct addrinfo hints, *ptr, *res;
    rxb_t rxb;
    char response[MAX_REQUEST_SIZE];
    char localita[MAX_REQUEST_SIZE];   // Regione di interesse
    char numero_loc[MAX_REQUEST_SIZE]; // Numero di località da mostrare
    char ack[3];                       // OK o NO
    size_t response_len, ack_len;

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

    /* Gestione richieste utente */
    for (;;)
    {

        /* Richiedo le informazioni all'utente */
        memset(localita, 0, sizeof(localita));
        memset(numero_loc, 0, sizeof(numero_loc));
        // Località
        printf("Inserire la località di interesse:\n");
        if (fgets(localita, sizeof(localita), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        /* Controllo che l'utente non voglia uscire */
        if (localita[0] == '.') {
            break;
        }

        // Numero delle località
        printf("Inserire il numero delle località di interesse:\n");
        if (fgets(numero_loc, sizeof(numero_loc), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        /* Controllo che l'utente non voglia uscire */
        if (localita[0] == '.') {
            break;
        }

        /* Invio le informazioni al server */
        // Nome località
        if (write_all(sd, localita, strlen(localita)) < 0)
        {
            perror("write all 1");
            exit(EXIT_FAILURE);
        }

        /* Ricevo ack */
        memset(ack, 0, sizeof(ack));
        ack_len = sizeof(ack) - 1;
        if (rxb_readline(&rxb, sd, ack, &ack_len) < 0)
        {
            fprintf(stderr, "Errore. Il server ha chiuso la connessione\n");
            rxb_destroy(&rxb);
            exit(EXIT_FAILURE);
        }

        /* Controllo ack */
        if (strcmp(ack, "OK") != 0)
        {
            fprintf(stderr, "Errore.");
            exit(EXIT_FAILURE);
        }

        // Invio Numero località
        if (write_all(sd, numero_loc, strlen(numero_loc)) < 0)
        {
            perror("write all 2");
            exit(EXIT_FAILURE);
        }

        /* Ricevo il risultato */
        printf("\ncm di neve, nome località, comprensorio sciistico\n");
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

            /* Stampo la risposta a video */
            puts(response);

            /* Controllo fine ricezione */
            if (strcmp(response, "FINE") == 0)
            {
                break;
            }
        }
        printf("\n");
    }
    /* Chiudo la socket */
    close(sd);

    return 0;
}
