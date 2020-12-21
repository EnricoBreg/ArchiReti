#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "rxb.h"
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64*1024; // Richiesta massima 64 Kbyte

// Usage: ./rgrep_client nomehost porta
int main(int argc, char const *argv[])
{
    int err, sd;
    struct addrinfo hints, *ptr, *res;
    rxb_t rxb_buffer;

    // Controllo argomenti
    if (argc != 3) {
        fprintf(stderr, "Errore. Uso corretto: %s nomehost\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Preparo direttive per getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // Invocazione getaddrinfo
    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err == -1) {
        fprintf(stderr, "Errore in getaddrinfo: %s", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Connessione con fallback
    for(ptr=res; ptr != NULL; ptr=ptr->ai_next){
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sd < 0) continue;

        if(connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) 
            break; // Connessione riuscita
       
        close(sd);
    }
    // Controllo che il client sia connesso
    if(ptr == NULL) {
        fprintf(stderr,"Attenzione. Il client non è connesso\n");
        exit(EXIT_FAILURE);
    }

    // Deallocazione riservata a getaddrinfo
    freeaddrinfo(res);

    // Inizializzo il buffer
    rxb_init(&rxb_buffer, MAX_REQUEST_SIZE);

    // Il client è connesso invio al server il nome del file
    for(;;) {
        // Buffer per nomefile da inviare al server
        char nomefile[2048], stringa[2048];
        char exist;
        // Richiesta nomefile all'utente
        printf("Inserisci il nome del file:\n");
        if (fgets(nomefile, sizeof(nomefile), stdin) == NULL) {
            perror("Errore fgets 1");
            exit(EXIT_FAILURE);
        }
        // Se l'utente inserisce . esco
        if (strcmp(nomefile, ".\n") == 0) {
            break;
        }
        // Invio il nome file al server
        if (write_all(sd, nomefile, strlen(nomefile)) < 0) {
            perror("Errore write 1");
            exit(EXIT_FAILURE);
        }  

        

        // Lettura del flag S o N
        if (read_all(sd, &exist, sizeof(exist)) < 0) {
            perror("Errore read");
            exit(EXIT_FAILURE);
        }

        if(exist == 'S') { // Se il file esiste
            // Richiesta della stringa all'utente
            printf("Inserisci la stringa da cercare:\n");
            if (fgets(stringa, sizeof(stringa), stdin) == NULL) {
                perror("Errore fgets 2");
                exit(EXIT_FAILURE);
            }
            // Verifico che l'utente non abbia inserito .
            if (strcmp(stringa, ".\n") == 0) {
                break;
            }
            // invio la stringa al server
            if (write_all(sd, stringa, strlen(stringa)) < 0) {
                perror("Errore write 2");
                exit(EXIT_FAILURE);
            }

            // Lettura della risposta dal server
            for(;;)  {
                char response[MAX_REQUEST_SIZE];
                size_t response_len;

                // Inizializzo il buffer a NULL
                memset(response, 0, sizeof(response));
                response_len = sizeof(response)-1;

                // Ricevo il risultato della grep dal server
                if (rxb_readline(&rxb_buffer, sd, response, &response_len) < 0) {
                    // Il server ha chiuso la connessione
                    rxb_destroy(&rxb_buffer);
                    fprintf(stderr, "Attenzione. Il server ha chiuso la connessione. Impossibile proseguire con la richiesta\n");
                    exit(EXIT_FAILURE);
                }

                // Stampo la riga a video
                puts(response);
                // Passo ad una nuova richiesta una volta che il server ha finito di inviare
                if (strcmp(response, "FINE") == 0) {
                    break;
                }
            }
        }
        else {
            // il file non esiste, lo comunico all'utente ed esco
            fprintf(stderr, "Il file non esiste\n");
            exit(EXIT_FAILURE);
        }
    }

    // Chiudo la socket
    close(sd);
    // Termino
    return 0;
}
