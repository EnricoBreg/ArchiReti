#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
} OUT_TYPE;

// Uso: ./rps_client nomehost porta [args for ps]
int main(int argc, char const *argv[])
{
    int sd, err, nread;
    char buff_res[1500];
    struct addrinfo hints, *res, *ptr;

    if(argc > 4) {
        fprintf(stderr, "Errore uso. Uso corretto: %s nomehost porta [args for ps]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Preparazione per le direttive di getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if(err != 0) {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }   

    // Preparo la connessione con fallback
    for(ptr=res; ptr != NULL; ptr=ptr->ai_next) {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sd < 0) {
            /* Prossima iterazione */
            continue;
        }
        /* Connesione andata a buon fine */
        if(connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) {
            break;
        }
        /* Passo al nodo successivo della lista */
        close(sd);
    }

    // Controllo che la socket sia effettivamente connessa
    if(ptr == NULL) {
        fprintf(stderr, "Errore di connessione!\n");
        close(sd);
        exit(EXIT_FAILURE);
    }

    // Libero la memoria allocata per getaddrinfo
    freeaddrinfo(res);

    /* Caso opzionale: si mandino anche degli argomenti */
    if (argc ==  4) {
        if(write(sd, argv[3], strlen(argv[3])) < 0) {
            perror("errore write client");
            exit(EXIT_FAILURE);
        }
    }

    // Chiusura del lato in scrittura della socket, equivalente ad un flush della socket e un invio "forzato"
    shutdown(sd, SHUT_WR);
    
    // Leggo il risultato del server
    fflush(stdout);
    while(nread = read(sd, buff_res, sizeof(buff_res)) > 0) {
        if(write(STDOUT, buff_res, strlen(buff_res)) < 0) {
            perror("errore write");
            close(sd);
            exit(EXIT_FAILURE);
        }
    }

    printf("\n");
    /* Chiudo la socket */
    close(sd);

    /* Esco senza errori */
    return 0;
}
