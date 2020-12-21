#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
/* Per utilizzare libunistring */
#ifdef USE_LIBUNISTRING 
    #include <unistr.h>
#endif
#include "rxb.h"
#include "utils.h"

/* Lunghezza massima di un buffer rxb di 64 Kilobytes */
const unsigned int MAX_REQUEST_SIZE = 64*1024;

int main(int argc, char const *argv[])
{
    int err, sd;
    struct addrinfo hints, *res, *ptr;
    rxb_t rxb; // buffer di ricezione
    char request[MAX_REQUEST_SIZE];
    uint8_t response[MAX_REQUEST_SIZE];
    size_t response_len;

    if (argc != 3) {
        fprintf(stderr, "Errore. Uso corretto: %s nomehost porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Preparo le direttive per getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; 

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    // Connessione con fallback
    for(ptr=res; ptr != NULL; ptr=ptr->ai_next) {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        
        if (sd == -1) 
            continue; // Nodo della lista non valido, passo al successivo

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) 
            break; // La connessione è riuscita, posso uscire dal ciclo
        
        close(sd);
    }
    // Controllo che il clienti sia effettivamente connesso
    if (ptr == NULL) {
        fprintf(stderr, "Errore. Il client non è connesso!\n");
        exit(EXIT_FAILURE);
    }
    // Deallocazione della memoria allocata a getaddrinfo
    freeaddrinfo(res);

    // Inizializzazione del buffer di ricezione
    rxb_init(&rxb, MAX_REQUEST_SIZE);

    for(;;) {

        // Lettura della stringa da tastiera
        printf("Inserisci una stringa:\n");
        if (fgets(request, sizeof(request), stdin) == NULL) { // Controllo della corretta esecuzione di fgets()
            fprintf(stderr, "Errore inaspettato in fgets()\n");
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // Invio la richiesta al server
        if (write_all(sd, request, sizeof(request)) < 0) {
            perror("Errore write_all()");
            exit(EXIT_FAILURE);
        }

        // Inizializzazione del buffer a zero, viene lasciato l'ultimo carattere a zero 
        // così da poterlo interpretare come
        // una stringa null terminated C
        memset(response, 0, sizeof(response));
        response_len = sizeof(response)-1;

        // Ricevo il risultato
        if (rxb_readline(&rxb, sd, response, &response_len) < 0) {
            // Se sono qui il server ha chiuso la connessione
            rxb_destroy(&rxb);
            fprintf(stderr, "Errore. Connessione chiusa dal server!\n");
            exit(EXIT_FAILURE);
        }

        printf("Risposta ottenuta dal server: %s\n", response);
    }

    close(sd);
    
    return 0;
}
