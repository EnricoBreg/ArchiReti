#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "utils.h"
#include "rxb.h"

const unsigned int MAX_SIZE_REQUEST = 64*1024;

// ./client host porta
int main(int argc, char const *argv[])
{
    int i, sd, err;
    struct addrinfo hints, *res, *ptr;
    char nome_vino[MAX_SIZE_REQUEST], annata_vino[MAX_SIZE_REQUEST];

    // Controllo argomenti
    if (argc != 3) {
        fprintf(stderr, "Errore. uso corretto: %s host porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err < 0) {
        fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    for(ptr = res, i = 0; ptr != NULL; ptr = ptr->ai_next, i++) {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        
        if (sd < 0) continue;

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) break; // Connessione riuscita

        close(sd);
    }
    
    if (ptr == NULL) {
        fprintf(stderr, "Connessione non riuscita\n");
        exit(EXIT_FAILURE);
    }

    printf("Connessione riuscita al tentantivo n. %d\n\n", i);

    for(;;) {

        printf("Inserire il nome del vino:\n");
        if (fgets(nome_vino, sizeof(nome_vino), stdin) == NULL) {
            perror("gets");
            exit(EXIT_FAILURE);
        }

        if (fgets(annata_vino, sizeof(nome_vino), stdin) == NULL) {
            perror("gets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(annata_vino, ".\n")) {
            break;
        }

        if (write(sd, nome_vino, strlen(nome_vino)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        if (strcmp(annata_vino, ".\n")) {
            break;
        }

        if (write(sd, annata_vino, strlen(annata_vino)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Lettura dei risultati dal server
        for(;;) {
            int nread;
            char result[1500];
            
            memset(result, 0, sizeof(res));

            nread = read(sd, result, sizeof(result));
            if (nread < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            if (write(1, result, strlen(result)) < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        
    }
    // Chiusura della socket
    close(sd);

    return 0;
}
