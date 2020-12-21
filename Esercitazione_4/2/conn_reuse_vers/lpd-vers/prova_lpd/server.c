#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/sendfile.h>
#include <errno.h>

ssize_t write_all(int fd, const void *vptr, size_t n);
ssize_t read_all(int fd, void *vptr, size_t n);

int main(int argc, char const *argv[])
{
	int err, sd, on, ns;
	struct addrinfo hints;
	struct addrinfo *result;
	struct sigaction sa;
	pid_t pid_f;

	if (argc != 2)
	{
		fprintf(stderr, "Errore. Uso corretto: %s numeroporta\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Connessione sulla porta */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, argv[1], &hints, &result);
	if (err != 0)
	{
		fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
		exit(EXIT_FAILURE);
	}

	sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sd == -1)
	{
		perror("Errore socket()");
		exit(EXIT_FAILURE);
	}

	/* Disabilo attesa uscita fase TIME_WAIT prima di creazioe socket */
	on = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* Successo, esco dal ciclo */
	if (bind(sd, result->ai_addr, result->ai_addrlen) != 0)
	{
		perror("Errore bind");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result); /* Libero la memoriaMAX_REQUEST_SIZE allocata per getaddrinfo */

	if (listen(sd, SOMAXCONN) < 0)
	{
		perror("Errore listen");
		exit(EXIT_FAILURE);
	}

	printf("SERVER CONNESSO IN ATTESA DI RICHIESTE...\n");

	for (;;)
	{
		int string_len, ns, nread;
		int len[2];
		char string[100];
		char *response = "HO RICEVUTO TUTTO\n"; 

		ns = accept(sd, NULL, NULL);
		if (ns < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// Leggo lunghezza stringa 
		memset(&len, 0, sizeof(len));
		if ((nread = read_all(ns, len, 2)) < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		
		// Decodifico la lunghezza
		string_len = (len[0] << 8) | len[1];

		// Leggo la stringa
		memset(string, 0, sizeof(string));
		if ((nread = read_all(ns, string, string_len)) < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		if (write_all(ns, response, strlen(response)) < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		break;
	}

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