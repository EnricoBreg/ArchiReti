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

ssize_t write_all(int fd, const void *vptr, size_t n);
ssize_t read_all(int fd, void *vptr, size_t n);

int main(int argc, char const *argv[])
{
	int sd, err, nread;
	int len[2];
	char *string = "HELLO GUYS";
	char response[100];
	struct addrinfo hints, *ptr, *res;
	size_t string_len;

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

	// Lunghezza stringa
	string_len = strlen(string);

	// Codifica della lunghezza
	len[0] = (string_len & 0xFF00) >> 8;
	len[1] = (string_len & 0x00FF);

	// Invio lunghezza al server 
	if (write_all(sd, len, 2) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	// Invio stringa
	if (write_all(sd, string, strlen(string)) < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	// Leggo la risposta 
	if ((nread = read_all(sd, response, sizeof(response))) < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	printf("%s\n", response);


    return 0;
}

ssize_t write_all(int fd, const void *vptr, size_t n)
{
        size_t remaining;
        ssize_t cc;
        const uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = write(fd, ptr, remaining)) <= 0) {
                        if (cc < 0 && errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                }

                remaining -= cc;
                ptr += cc;
        }
        return n;
}

ssize_t read_all(int fd, void *vptr, size_t n)
{
	size_t remaining;
	ssize_t cc;
        uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = read(fd, ptr, remaining)) < 0) {
                        if (errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                } else if (cc == 0) /* EOF */
                        break;

                remaining -= cc;
                ptr += cc;
        }
        return (n - remaining);
}