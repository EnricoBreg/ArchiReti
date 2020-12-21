#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>      // addrinfo ...
#include <string.h>     // memset ...
#include <sys/socket.h> // connect ...
#include <unistd.h>     // write, read ...
#include <errno.h>      // perror

#define _POSIX_C_SOURCE 200809L

const int MAX = 2048;

// ./rstrlen_client nomehost porta
int main(int argc, char const *argv[])
{
    int err, sd, nread;
    char stringa[MAX], len[MAX];
    struct addrinfo hints, *res, *ptr;

    // Controllo argomenti
    if (argc != 3)
    {
        fprintf(stderr, "Errore. Uso corretto: ./rstrlen_client nomehost porta\n");
        exit(-1);
    }

    // Preparo le strutture per la funzione getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Invocazione di gretaddrinfo
    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err < 0)
    {
        perror("Errore getaddrinfo");
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(-2);
    }

    // Richiesta di una nuova stringa dall'utente
    printf("Inserire una stringa: ");
    if (fgets(stringa, sizeof(stringa), stdin) == NULL)
    {
        perror("errore fgets");
        exit(-4);
    }

    while (strcmp(stringa, "fine") != 0)
    {
        // Connessione con fallback
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
        {
            sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            // Se socket fallisce passo all'indirizzo successivo
            if (sd < 0)
            {
                continue;
            }

            // Se la socket va a buon fine esce dal ciclo
            if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            {
                break;
            }

            // Altrimenti chiudo la socket e passo all'indirizzo successivo
            shutdown(sd, SHUT_RDWR);
        }

        // Connessione riuscita, posso mandare la stringa letta al server
        if (write(sd, stringa, strlen(stringa)) < 0)
        {
            perror("Errore write");
            close(sd);
            exit(-3);
        }

        // Lettura della risposta, read bloccante
        memset(len, 0, sizeof(len));
        nread = read(sd, len, sizeof(len) - 1);
        if (nread == -1)
        {
            perror("errore read");
        }
        // stampo il risultato
        printf("Num. di caratteri delle stringa inserita: %s\n", len);

        // chiusura della socket
        close(sd);

        // Richiesta di una nuova stringa dall'utente
        printf("Inserire una stringa: ");
        if (fgets(stringa, sizeof(stringa), stdin) == NULL)
        {
            perror("errore fgets");
            exit(-4);
        }
    }

    // ANSI C requires main function returns zero value
    return 0;
}
