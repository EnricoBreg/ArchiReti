#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define _POSIX_C_SOURCE 200809L

const unsigned int MAX_REQUEST_SIZE = 2048;
const unsigned int MAX_RESPONSE_SIZE = 2;

// USAGE: ./rstrcmp_client nomeserver nomeporta stringa1 stringa2
int main(int argc, char const *argv[])
{
    int err, sd, errno, length_of_string1, length_of_string2, nread;
    char request[MAX_REQUEST_SIZE], response[MAX_RESPONSE_SIZE];
    struct addrinfo hints, *res, *ptr;

    // Controllo del numero di argomenti
    if (argc != 5)
    {
        fprintf(stderr, "Errore. Uso corretto: %s nomeserver numeroporta stringa1 stringa2\n", argv[0]);
        exit(-1);
    }

    // Preparazione delle direttive per getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "Error in getaddrinfo function: %s", gai_strerror(err));
        exit(-2);
    }

    // Preparo la connessione con fallback
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        /* Se la socket fallisce passa al nodo seguente della lista */
        if (sd < 0)
            continue;

        /* Se invece va a buon fine connetti con il server */
        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            /* La connessione è riuscita, ora posso uscire dal ciclo for di connessioni*/ 
            break;  
        }

        /* Altrimenti, passo all'indirizzo successivo */
        close(sd);
    }
    /* Libero la memoria allocata a getaddrinfo */
    freeaddrinfo(res);

    /**Preparo il pacchetto da mandare al server con sprintf
     * Il pacchetto comprende:
     * Lunghezza della prima stringa seguito dalla prima stringa
     * Lunghezza della seconda stringa seguita dalla seconda stringa
     * Esempio di pacchetto:
     * stringa1 = "ciao"
     * stringa2 = "bella"
     * pacchetto: 4ciao5bella */
    memset(request, 0, sizeof(request));
    length_of_string1 = strlen(argv[3]);
    length_of_string2 = strlen(argv[4]);

    sprintf(request, "%d%s%d%s", length_of_string1, argv[3], length_of_string2, argv[4]);

    /* Invio la richiesta al server */
    if(write(sd, request, strlen(request)) < 0) {
        perror("errore write");
        close(sd);
        exit(-3);
    }

    /* Leggo la risposta dal server */
    nread = read(sd, response, sizeof(response));
    if(nread < 0) {
        perror("errore read");
        close(sd);
        exit(-4);
    }

    /* Stampo il risultato a video */
    if(strcmp("SI", response) == 0) {
        printf("\nLe stringhe inserite sono uguali!\n");
    } else {
        printf("Le stringhe inserite sono diverse!\n");
    }

    // ANSI C main return value
    return 0;
}
