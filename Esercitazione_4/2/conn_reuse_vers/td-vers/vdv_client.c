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
    char annata_vino[MAX_REQUEST_SIZE];
    char nome_vino[MAX_REQUEST_SIZE];
    char ack[3]; // OK o NO
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

    rxb_init(&rxb, MAX_REQUEST_SIZE);

    for (;;)
    {
        printf("Inserire nome vino:\n");
        if (fgets(nome_vino, sizeof(nome_vino), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (nome_vino[0] == '.')
        {
            break;
        }

        printf("Inserisci annata vino:\n");
        if (fgets(annata_vino, sizeof(annata_vino), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (annata_vino[0] == '.')
        {
            break;
        }

        /* Invio nome vino */
        if (write_all(sd, nome_vino, strlen(nome_vino)) < 0)
        {
            perror("write all 1");
            exit(EXIT_FAILURE);
        }

        /* Ricevo ack */
        memset(ack, 0, sizeof(ack));
        ack_len = sizeof(ack) - 1;
        if (rxb_readline(&rxb, sd, ack, &ack_len) < 0)
        {
            rxb_destroy(&rxb);
            fprintf(stderr, "Il server ha chiuso la connessione");
            exit(EXIT_FAILURE);
        }

        if (strcmp(ack, "OK") != 0)
        {
            fprintf(stderr, "Errore");
            exit(EXIT_FAILURE);
        }

        /* Invio annata vino */
        if (write_all(sd, annata_vino, strlen(annata_vino)) < 0)
        {
            perror("write all 2");
            exit(EXIT_FAILURE);
        }

        /* Ricevo il risultato */
        for (;;)
        {
            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;
            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                rxb_destroy(&rxb);
                fprintf(stderr, "Il server ha chiuso la connessione");
                exit(EXIT_FAILURE);
            }

            puts(response);

            /* Fine input */
            if (strcmp(response, "FINE") == 0) {
                break;
            }
        }
    }

    // Chiudo la socket
    rxb_destroy(&rxb);
    close(sd);

    return 0;
}
