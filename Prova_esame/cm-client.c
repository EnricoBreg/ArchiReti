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

/* uso: ./cm-client host porta*/
int main(int argc, char const *argv[])
{
    int sd, err;
    struct addrinfo hints, *ptr, *res;
    rxb_t rxb;
    char response[MAX_REQUEST_SIZE];
    char username[1024];
    char password[1024];
    char categoria[MAX_REQUEST_SIZE];
    char ack[3], autor[3]; // OK o NO
    size_t response_len, ack_len, autor_len;

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

    /* Inizializzazione del buffer rx */
    rxb_init(&rxb, MAX_REQUEST_SIZE);

    /* Richiedo username e password*/
    printf("Inserire username:\n");
    if (fgets(username, sizeof(username), stdin) == NULL)
    {
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    printf("Inserire password:\n");
    if (fgets(password, sizeof(password), stdin) == NULL)
    {
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    /* Invio username */
    if (write_all(sd, username, strlen(username)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    /* Ricevo e controllo ack */
    memset(ack, 0, sizeof(ack));
    ack_len = sizeof(ack) - 1;
    if (rxb_readline(&rxb, sd, ack, &ack_len) < 0)
    {
        fprintf(stderr, "Errore");
        rxb_destroy(&rxb);
        exit(EXIT_FAILURE);
    }

    if (strcmp(ack, "OK") != 0) {
        fprintf(stderr, "Si è ferificato un errore\n");
        exit(EXIT_FAILURE);
    }

    /* Debugging */
    //puts(ack);

    /* Invio password */
    if (write_all(sd, password, strlen(password)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    memset(autor, 0, sizeof(autor));
    autor_len = sizeof(autor) - 1;
    if (rxb_readline(&rxb, sd, autor, &autor_len) < 0)
    {
        fprintf(stderr, "Errore");
        rxb_destroy(&rxb);
        exit(EXIT_FAILURE);
    }

    /* Controllo di essere autorizzato */
    if (strcmp(autor, "SI") != 0)
    {
        fprintf(stderr, "Non sei autorizzato!\n");
        exit(EXIT_FAILURE);
    }

    /* Gestione delle richieste */
    for (;;)
    {
        printf("\nInserire la categoria ('fine' per uscire):\n");
        if (fgets(categoria, sizeof(categoria), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        /* Controllo che l'utente non abbia inserito 'fine' */
        if (strcmp(categoria, "fine\n") == 0)
        {
            break;
        }

        /* Invio la categoria al server */
        if (write_all(sd, categoria, strlen(categoria)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        /* Ricevo il risultato */
        for (;;)
        {
            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;
            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                fprintf(stderr, "Errore. Il server ha chiuso la connessione\n");
                rxb_destroy(&rxb);
                exit(EXIT_FAILURE);
            }
            /* Controllo il valore di response */
            if (strcmp(response, "FINE") == 0)
            {
                break;
            }
            /* Stampo a video*/
            puts(response);
        }
    }

    /* Chiudo la socket */
    rxb_destroy(&rxb);
    close(sd);

    return 0;
}
