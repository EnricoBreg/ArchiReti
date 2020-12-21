#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 4096

int main(int argc, char *argv[])
{
        int err;
        struct addrinfo hints, *res;
        char *host_remoto, *servizio_remoto;
	char username[BUFSIZE];
	char password[BUFSIZE];
	char categoria_macchine[BUFSIZE];

        /* Controllo argomenti */
        if (argc < 3) {
                fprintf(stderr, "Uso: coffee_machines <server> <porta>\n");
                exit(EXIT_FAILURE);
        }

        /* Costruzione dell'indirizzo */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /* Risoluzione dell'host */
        host_remoto = argv[1];
        servizio_remoto = argv[2];

        if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0) {
                fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        printf("Inserisci username:\n");
        scanf("%s", username);

        do {
                int sd, cc, i=0;
                struct addrinfo *ptr;
                uint8_t buffer[BUFSIZE];

                printf("Inserisci numero giochi:\n");
                scanf("%s", password);

                printf("Inserisci tipologia giochi:\n");
                scanf("%s", categoria_macchine);

                /* Connessione con fallback */
                for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
                    /* Se socket fallisce salto direttamente alla prossima iterazione */
                    if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0){
                        fprintf(stderr,"creazione socket fallita\n");
                        continue;
                    }
                    /* Se connect funziona esco dal ciclo */
                    if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0){
                        printf("connect riuscita al tentativo %d\n",i);
                        break;
                    }
                    i++;
                    close(sd);
                }

                /* Verifica sul risultato restituito da getaddrinfo */
                if (ptr == NULL) {
                    fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato\n");
                    exit(3);
                }

                /* Scambio dati con il server: */

                /* 1) invio username */
                if (write(sd, username, strlen(username)) < 0) {
                        perror("write username");
                        exit(EXIT_FAILURE);
                }

                /* 2) lettura ACK */
                if (read(sd, buffer, sizeof(buffer)) < 0) {
                        perror("read ack");
                        exit(EXIT_FAILURE);
                }

                /* 3) invio password */
                if (write(sd, password, strlen(password)) < 0) {
                        perror("write password");
                        exit(EXIT_FAILURE);
                }

                /* 4) lettura ACK */
                if (read(sd, buffer, sizeof(buffer)) < 0) {
                        perror("read ack");
                        exit(EXIT_FAILURE);
                }

                /* 5) invio categoria_macchine */
                if (write(sd, categoria_macchine, strlen(categoria_macchine)) < 0) {
                        perror("write categoria_macchine");
                        exit(EXIT_FAILURE);
                }

                /* Lettura risultato */
                while ((cc = read(sd, buffer, sizeof(buffer))) > 0) {
                        write(1, buffer, cc);
                }

                if (cc < 0) {
                        perror("read risultato da server");
                        exit(EXIT_FAILURE);
                }

                /* Dimenticarsi di chiudere la socket è un errore molto grave!!! */
                close(sd);

                printf("Inserisci username (fine per terminare):\n");
                scanf("%s", username);
        } while(strcmp(username, "fine"));

        /* Liberiamo la memoria allocata da getaddrinfo */
        freeaddrinfo(res);

        return 0;
}
