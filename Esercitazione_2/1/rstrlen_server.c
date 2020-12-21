#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#define MAX 2048

int main(int argc, char const *argv[])
{   
    int err, sd;
    struct addrinfo hints, *res;

    // Controllo argomenti
    if(argc != 2){
        fprintf(stderr, "Errore . Uso corretto: %s numeroporta", argv[0]);
        exit(-1);
    }

    // Preparo le direttive per addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, argv[1], &hints, &res);
    if(err != 0) {
        fprintf(stderr, "Errore setup getaddrinfo bin: %s\n", gai_strerror(err));
        exit(-2);
    }

    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sd < 0) {
        perror("Errore socket");
        exit(-3);
    }

    // Metto in ascolto la socket sulla porta desiderata
    if(bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("errore in bind");
        close(sd);
        exit(-4);
    }

    // trasformo la socket attiva in passiva per il server
    if(listen(sd, SOMAXCONN) < 0) {
        perror("Errore listen");
        exit(-5);
    }

    printf("SERVER CONNESSO\n");

    // gestione delle richieste
    for(;;) {
        int ns, nread, length_of_string;
        char request[MAX], response[MAX];

        // accept restituisce una socket attiva dalla socket passiva del server
        ns = accept(sd, NULL, NULL);
        if(ns < 0) {
            if(errno == EINTR) {
                continue;
            }
            perror("Errore in accept");
            close(sd);
            exit(-6);
        }

        // Lettura della richiesta dell'utente
        memset(request, 0, sizeof(request));
        nread = read(ns, request, sizeof(request)-1);
        if(nread < 0) {
            perror("errore read");
            continue;
        }

        // elaborazione della richiesta
        length_of_string = strlen(request)-1;

        snprintf(response, sizeof(response), "%d\n", length_of_string);

        // Invio la risposta al server
        if(write(ns, response, sizeof(response)) < 0) {
            perror("errore write");
            close(ns);
            continue;
        }

        // chiusura della socket attiva
        close(ns);        
    }

    // Chiusura della socket passiva del server, in teoria questa istruzione non viene mai raggiunta
    close(sd);

    return 0;
}
