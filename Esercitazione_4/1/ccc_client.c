#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

// ./ccc_client nomehost porta
int main(int argc, char const *argv[])
{
    int err, sd, nread;
    uint8_t categoria_spesa[100], buff[1500];
    struct addrinfo hints, *res, *ptr;

    if (argc != 3)
    {
        fprintf(stderr, "Errore. uso corretto: %s nomehost porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    memset(categoria_spesa, 0, sizeof(categoria_spesa));
    printf("Inserisci una categoria di spesa da cercare: ");
    if (fgets(categoria_spesa, sizeof(categoria_spesa), stdin) < 0) {
        perror("Errore fgets()");
        exit(EXIT_FAILURE);
    }

    while (strncmp(categoria_spesa, "fine", 4) != 0) {
        
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
            sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (sd == -1)
            {
                /* Passa all'indirizzo successivo */
                continue;
            }

            if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            {
                /* La connessione è riuscita posso uscire dal ciclo */
                break;
            }
            close(sd);
        }

        if (ptr == NULL) {
            fprintf(stderr, "Errore. Il client non è connesso!\n");
            exit(EXIT_FAILURE);
        }
        
        /* Invio categoria spesa al server */
        if (write(sd, categoria_spesa, strlen(categoria_spesa)) < 0) {
            perror("errore write categoria spesa");
            exit(EXIT_FAILURE);
        }

        /* Il client è connesso di mette in attesa di ricevere dati dal server */
        memset(buff, 0, sizeof(buff));
        while ((nread = read(sd, buff, sizeof(buff))) > 0) { /* Leggo da socket... */
            if (write(1, buff, nread) < 0) { /* ... e scrivo a video */
                perror("errore write");
                exit(EXIT_FAILURE);
            }
            memset(buff, 0, sizeof(buff));
        }

        printf("\n");
        /* Chiuso la connessione con il server */
        close(sd);

        memset(categoria_spesa, 0, sizeof(categoria_spesa)); /* Pulizia del buffer */
        /* Nuova richiesta */
        printf("Inserisci una categoria di spesa da cercare: ");
        if (fgets(categoria_spesa, sizeof(categoria_spesa), stdin) < 0) {
            perror("Errore fgets()");
            exit(EXIT_FAILURE);
        }
    } /* Fine while */

    return 0;
}
