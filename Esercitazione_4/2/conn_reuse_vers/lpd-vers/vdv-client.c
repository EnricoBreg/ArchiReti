#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

int main(int argc, char const *argv[])
{
    uint8_t buffer[2048];
	uint8_t len[2];
	char nome_vino[512];
	char annata_vino[512];
	int nomevino_len;
	int annatavino_len;
	int sd, err, nread;
	struct addrinfo hints, *ptr, *res;

	if (argc != 3)
	{
		fprintf(stderr, "Sintassi: %s hostname porta\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(argv[1], argv[2], &hints, &res);
	if (err != 0)
	{
		fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* connessione con fallback */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
	{
		sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		/* se la socket fallisce, passo all'indirizzo successivo */
		if (sd < 0)
			continue;

		/* se la connect va a buon fine, esco dal ciclo */
		if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
			break;

		/* altrimenti, chiudo la socket e passo all'indirizzo
                 * successivo */
		close(sd);
	}

	/* controllo che effettivamente il client sia connesso */
	if (ptr == NULL)
	{
		fprintf(stderr, "Errore di connessione!\n");
		exit(EXIT_FAILURE);
	}

	/* a questo punto, posso liberare la memoria allocata da getaddrinfo */
	freeaddrinfo(res);

    /* Gestione richieste utente */
    for(;;) {
        /* Chiedo il nome del vino */
        printf("Inserire il nome del vino ('fine' per uscire):\n");
        if (scanf("%s", nome_vino) == EOF) {
            perror("scanf");
            exit(EXIT_FAILURE);
        }

        /* Controllo valore di nome_vino */
        if (strcmp(nome_vino, "fine") == 0) {
            break;
        }
        /* Chiedo annata del vino */
        printf("Inserire l'annata del vino ('fine' per uscire):\n");
        if (scanf("%s", annata_vino) == EOF) {
            perror("scanf");
            exit(EXIT_FAILURE);
        }

        /* Controllo valore di nome_vino */
        if (strcmp(annata_vino, "fine") == 0) {
            break;
        }

        /* Per non aver problemi con write */
		fflush(stdout);
       
        /* Invio il nome del vino al server */
        nomevino_len = strlen(nome_vino);
        if (nomevino_len > UINT16_MAX) {
            fprintf(stderr, "Errore. Nome vino troppo lungo\n");
            exit(EXIT_FAILURE);
        }

        /* Codifica della lunghezza */
        len[0] = (nomevino_len & 0xFF00) >> 8;
        len[1] = (nomevino_len & 0x00FF);
        /* Invio della lunghezza */
        if (write_all(sd, len, 2) < 0) {
            perror("write all");
            exit(EXIT_FAILURE);
        }

        /* Invio il nome del vino */
        if (write_all(sd, nome_vino, strlen(nome_vino)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        /* Invio annata vino al server */
        annatavino_len = strlen(annata_vino);

        /* Codifica della lunghezza */
        len[0] = (annatavino_len & 0xFF00) >> 8;
        len[1] = (annatavino_len & 0x00FF);
        /* Invio della lunghezza */
        if (write_all(sd, len, 2) < 0) {
            perror("write all");
            exit(EXIT_FAILURE);
        }

        /* Invio annata del vino */
        if (write_all(sd, annata_vino, strlen(annata_vino)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        /* Leggo la lunghezza della risposta */
        if (read_all(sd, len, 2) < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* Ricevo la risposta e la stampo a video */
        int resp_len = len[0] << 8 | len[1];
        int to_read = resp_len;

        printf("\n%d", to_read);

        while(to_read > 0) {
            size_t buffer_size = sizeof(buffer);
            size_t sz = (to_read < buffer_size) ? to_read : buffer_size;

            nread = read(sd, buffer, sz);
            if (nread < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            if (write_all(1, buffer, nread) < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            /* bytes da leggere ancora */
            to_read -= nread;
        }
        
        printf("\n");
    }

    /* Chiudo la socket */
    close(sd);


    return 0;
}
