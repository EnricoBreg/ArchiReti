#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include "rxb.h"
#include "utils.h"

// Dimensione massima del buffer di richiesta 64 Kbyte
const unsigned int MAX_REQUEST_SIZE = 64*1024;

/* Controllo argomenti */
int main(int argc, char const *argv[])
{
    int err, sd;
    struct addrinfo hints, *ptr, *res;
    rxb_t rxb_buffer;
    

    /* Controllo argomenti */
    if (argc < 2) {
        fprintf(stderr, "Errore. Uso corretto: %s nomehost porta\n", argv[0]);
        exit(EXIT_FAILURE);        
    }
    // Preparazione per getaddrifo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // Invocazione per getaddrinfo
    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err == -1) {
        fprintf(stderr, "Errore in gettaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    // Connessione con fallback
    for(ptr = res; ptr != NULL; ptr=ptr->ai_next) {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sd < 0) {
            continue;
        }
        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) {
            break;
        }
        close(sd); 
    }
    // Controllo che il client sia effettivamente connesso
    if (ptr == NULL) {
        fprintf(stderr, "Errore. Il client non è connesso\n");
        exit(EXIT_FAILURE);
    }
    // Deallocazione memeoria riservata a getaddrinfo
    freeaddrinfo(res);

    // Preparo il buffer
    rxb_init(&rxb_buffer, MAX_REQUEST_SIZE);

    for(;;) {
        // Buffer per le opzioni per il comando ps
        char ps_options[1024];
        // Richiesta dei paramentri all'utente
        printf("Inserisci i parametri per ps:\n");
        if (fgets(ps_options, sizeof(ps_options), stdin) == NULL) {
            perror("Errore fgets");
            exit(EXIT_FAILURE);
        }
        // Se l'utente inserisce . esco
        if (strcmp(ps_options, ".\n") == 0) {
            break;
        }
        // Invio la richiesta al server
        if (write_all(sd, ps_options, strlen(ps_options)) < 0) {
            perror("Errore write_all");
            exit(EXIT_FAILURE);
        }
        // Lettura della risposta dal server
        for(;;) {
            char response[MAX_REQUEST_SIZE];
            size_t response_len;

            // Inizializzazione del buffer a null
            memset(response, 0, sizeof(response));
            response_len = sizeof(response)-1;

            // Ricevo il risultato dal server
            if (rxb_readline(&rxb_buffer, sd, response, &response_len) < 0) {
                // Se sono qui il server ha chiuso la connessione
                rxb_destroy(&rxb_buffer);
                fprintf(stderr, "Il server ha chiuso la connessione!\n");
                exit(EXIT_FAILURE);
            }
            // Stampo la riga a video
            printf("%s\n", response);
            // Passo ad una nuova richiesta una volta terminato l'input del server
            if (strcmp(response, "FINE") == 0) {
                break;
            }
        }
    }    
    // Chiudo la socket
    close(sd);
    // Esco
    return 0;
}
