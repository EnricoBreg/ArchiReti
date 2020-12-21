#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define DIM 4096

size_t parse_size(char *current);

int main(int argc, char** argv) {
        int err;
        struct addrinfo hints;
        struct addrinfo *res,*ptr;
        char *host_remoto;
        char *servizio_remoto;
        int sd, nread;
        char buf[DIM], nome[DIM], annata[DIM];
        int i=1;
        char *ackVer="ack\n";

        /* Controllo argomenti */
        if (argc < 3) {
                printf("Uso: %s <server> <porta>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        /* Costruzione dell'indirizzo */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        /* Risoluzione dell'host */
        host_remoto = argv[1];
        servizio_remoto = argv[2];

        if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0) {
                fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

	/* Connessione con fallback */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		/*se socket fallisce salto direttamente alla prossima iterazione*/
		if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
			fprintf(stderr,"creazione socket fallita\n");
			continue;
		}
		/*se connect funziona esco dal ciclo*/
		if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0) {
			printf("connect riuscita al tentativo %d\n",i);
			break;
		}
		i++;
		close(sd);
	}

	/* Verifica sul risultato restituito da getaddrinfo */
	if (ptr == NULL) {
		fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato\n");
		exit(EXIT_FAILURE);
	}

        /* Liberiamo la memoria allocata da getaddrinfo() */
        freeaddrinfo(res);

        for (;;) {
		char message[4096];
		int length_needed;

                /* Lettura dati dall'utente */
                printf("Inserisci il nome di un vino cui sei interessato ('fine' per uscire):\n");
                scanf("%s", nome); //ho il terminatore

                if (strcmp(nome, "fine") == 0) {
                        printf("Hai scelto di terminare il programma.\n");
                        break;
                }

                /* Lettura dati dall'utente */
                printf("Inserisci l'annata di un vino cui sei interessato ('fine' per uscire):\n");
                scanf("%s", annata); //ho il terminatore

                if (strcmp(annata, "fine") == 0) {
                        printf("Hai scelto di terminare il programma.\n");
                        break;
                }

                /* È consigliabile effettuare il flushing del buffer di printf
                 * prima di iniziare a scrivere sullo standard output con write */
                fflush(stdout);

		/* Preparo messaggio di richiesta */
		length_needed = snprintf(message, sizeof(message), "(%d:%s%d:%s)", 
		                         strlen(nome), nome, strlen(annata), annata);
		if (length_needed < 0 || length_needed >= sizeof(message)) {
                        fprintf(stderr, "Buffer 'message' troppo piccolo\n");
                        exit(EXIT_FAILURE);
		}

                /* Mando il messaggio al server */
                if (write(sd, message, strlen(message)) < 0) {
                        perror("write");
                        exit(EXIT_FAILURE);
                }

                /* Ricezione risultato */
		/* Leggo il messaggio canonical s-expression con 1 dato in un buffer */
		size_t left = sizeof(message);
		int bytes_received = 0;

		/* Verifico di avere (almeno, è possibile avere degli altri
		 * caratteri ':' nel contenuto della risposta) un carattere ':' */
		do {
			cc = read(sd, message + bytes_received, left);
			if (cc < 0) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			if (cc == 0) {
				fprintf(stderr, "Errore: connessione chiusa dal server!\n");
				exit(EXIT_FAILURE);
			}

			bytes_received += cc;
			left -= cc;

			/* Controllo se ho ricevuto il carattere ':' */
			if (memchr(message, ':', bytes_received+cc)) {
				break;
			}

			if (left == 0) {
				/* client che malfunziona - inviato
				 * messaggio di dimensioni maggiori di
				 * BUFSIZE */
				fprintf(stderr, "Error 1\n"); fflush(stderr);
				close(ns);
				exit(EXIT_SUCCESS);
			};
		} while (1);

		/* Verifico presenza parentesi */
		if (message[0] != '(')') {
			/* client che malfunziona - inviato messaggio
			 * senza parentesi */
			fprintf(stderr, "error 4\n"); fflush(stderr);
			close(ns);
			exit(EXIT_SUCCESS);
		}

		size = parse_size(message + 1);

		/* Offset from message to actual server input: need to consider
		 * character '(', string of length size, and character ':' */
		int offset = size + 2; 

		/* Impossibile, errore protocollo! */
		if (bytes_received - offset > size + 1) {
			fprintf(stderr, "Errore protocollo!\n");
			exit(EXIT_FAILURE);
		}

		/* Scrivo a video eventuali byte rimasti nel buffer */
		if (bytes_received - offset > 0) {
			if (write_all(sd, message + offset, bytes_received - offset) < 0) {
				perror("write");
				exit(EXIT_FAILURE);
			}
			size -= (bytes_received - offset);
		}

		/* Leggo dal Server i byte da leggere e li scrivo a video */
		left = size;
		while (left > 0) {
			/* Devo leggere il minimo tra left e dimensione del buffer message */
			int to_read = (left < sizeof(message) ? left : sizeof(message));

			nread = read(sd, message, to_read);
			if (nread < 0) {
				perror("read");
				exit(EXIT_FAILURE);
			}
                        if (write_all(1, message, nread) < 0) {
                                perror("write");
                                exit(EXIT_FAILURE);
                        }
                }

		/* Verifico presenza parentesi di chiusura */
		nread = read(sd, message, 1);
		if (nread < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		if (message[0] != ')') {
			/* Client che malfunziona - inviato messaggio
			 * senza parentesi di chiusura*/
			fprintf(stderr, "Errore protocollo!\n");
			exit(EXIT_FAILURE);
		}
        }

	/* Chiudo la socket */
	close(sd);

        return 0;
}


size_t parse_size(char *current)
{
	char *nextcolon;
	intmax_t size; 
	char *endptr;

	/* leggo anno_pubblicazione */
	nextcolon = strchr(current, ':');
	if (nextcolon == NULL) {
		/* client che malfunziona - inviato messaggio
		 * senza il primo carattere : */
		fprintf(stderr, "Error 5\n"); fflush(stderr);
		close(ns);
		exit(EXIT_SUCCESS);
	}

	/* TODO: extract string */
	char *response_size = (char *)malloc(nextcolon - current + 1);
	memcpy(response_size, current, nextcolon - current);
	response_size[nextcolon-current] = '\0';

	/*  
	 *  +--- current
	 *  |+-- nextcolon
	 *  vv
	 * (5:pippo7:plutone) 
	 */
	size = strtoimax(response_size, &endptr, 10);
	if (size == INTMAX_MAX && errno == ERANGE) {
		/* client che malfunziona - inviato messaggio
		 * con primo intero troppo alto */
		fprintf(stderr, "Error 6\n"); fflush(stderr);
		close(ns);
		exit(EXIT_FAILURE);
	}

	free(response_size);

	return size;
}
