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
#include "utils.h"

const unsigned int MAX_REQUEST_SIZE = 64 * 1024;
const char file_path[] = "/home/enrico/dirprova/magazzino.txt"; /* Percorso file magazzino.txt */

typedef int pipe_t[2];

void handler(int signo)
{
	int status;

	/* Padre, ricevuto segnale SIGCHLD */
	while (waitpid(-1, &status, WNOHANG) > 0)
	{
		continue;
	}
}

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

	/* Installo il gestore del segnale SIGCHLD */
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = handler;

	if (sigaction(SIGCHLD, &sa, NULL) < 0)
	{
		perror("sigaction");
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

	/* Gestione delle richieste */
	for (;;)
	{
		/* Accettazione della richiesta */
		ns = accept(sd, NULL, NULL);
		if (ns < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		/* Generazione del figlio per gestiore le richieste di un client */
		if ((pid_f = fork()) < 0)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid_f == 0)
		{
			/* FIGLIO */
			int fd, status, nomevino_len = 0, annatavino_len = 0;
			int len[2];
			char nome_vino[2048], annata_vino[2048], template[512];
			pid_t pid_n1, pid_n2;
			pipe_t n1n2;
			off_t response_len;

			/* Dinstallo il gestiore del segnale */
			signal(SIGCHLD, SIG_DFL);

			/* Chiudo la socket passiva */
			close(sd);

			/* Gestione richieste singolo client */
			for (;;)
			{
				/* Installo la pipe n1n2 */
				if (pipe(n1n2) < 0)
				{
					perror("pipe");
					exit(EXIT_FAILURE);
				}

				/* Lettura lunghezza nome vino */
				memset(&len, 0, sizeof(len));
				if (read_all(ns, len, 2) < 0)
				{
					perror("read");
					exit(EXIT_FAILURE);
				}

				/* Decodifica lunghezza nome vino */
				nomevino_len = len[1] | (len[0] << 8);

				/* Debugging */
				printf("%d\n", nomevino_len);

				/* Leggo nome vino */
				memset(nome_vino, 0, sizeof(nome_vino));
				if (read_all(ns, nome_vino, nomevino_len) < 0)
				{
					perror("read");
					exit(EXIT_FAILURE);
				}

				/* Debugging */
				printf("%s\n", nome_vino);

				/* Lettura lunghezza annata vino */
				memset(&len, 0, sizeof(len));
				if (read_all(ns, len, 2) < 0)
				{
					perror("read");
					exit(EXIT_FAILURE);
				}

				/* Decodifica lunghezza annata vino */
				annatavino_len = len[1] | (len[0] << 8);

				/* Debugging */
				printf("%d\n", annatavino_len);

				/* Leggo annata vino */
				memset(annata_vino, 0, sizeof(annata_vino));
				if (read_all(ns, annata_vino, annatavino_len) < 0)
				{
					perror("read");
					exit(EXIT_FAILURE);
				}

				/* Debugging */
				printf("%s\n", annata_vino);

				/* grep nome_vino file_path | grep annata_vino */

				/* Creo un file temporaneo che mi servirà per trovare la lunghezza
				 * dell'output che manderò al client */
				memset(template, 0, sizeof(template));
				strcpy(template, "/tmp/selected-wine-XXXXXX");
				fd = mkstemp(template);
				if (fd < 0)
				{
					perror("mkstemp");
					exit(EXIT_FAILURE);
				}

				/* Genero un figlio per execlp (grep nomevino) */
				if ((pid_n1 = fork()) < 0)
				{
					perror("fork");
					exit(EXIT_FAILURE);
				}
				else if (pid_n1 == 0)
				{
					/* NIPOTE 1 */

					/* Chiusura dei file descripton non usati */
					close(n1n2[0]);
					close(ns);
					close(fd);

					/* Redirezione dell'output */
					close(1);
					if (dup(n1n2[1]) < 0)
					{
						perror("dup");
						exit(EXIT_FAILURE);
					}
					close(n1n2[1]);

					/* execlp grep nomevino */
					execlp("grep", "grep", nome_vino, file_path, (char *)0);
					perror("execlp");
					exit(EXIT_FAILURE);
				}

				/* FIGLIO */

				/* Genero secondo figlio per execlp (grep annata vino) */
				if ((pid_n2 = fork()) < 0)
				{
					perror("fork");
					exit(EXIT_FAILURE);
				}
				else if (pid_n2 == 0)
				{
					/* NIPOTE 1 */

					/* Chiusura dei file descripton non usati */
					close(n1n2[1]);
					close(ns);

					/* Redirezione input */
					close(0);
					if (dup(n1n2[0]) < 0)
					{
						perror("dup");
						exit(EXIT_FAILURE);
					}
					close(n1n2[0]);

					/* Redirezione dell'output */
					close(1);
					if (dup(fd) < 0)
					{
						perror("dup");
						exit(EXIT_FAILURE);
					}
					close(fd);

					/* execlp grep nomevino */
					execlp("grep", "grep", annata_vino, (char *)0);
					perror("execlp");
					exit(EXIT_FAILURE);
				}

				/* FIGLIO */

				/* Chiusura del file descriptor non usati */
				close(n1n2[0]);
				close(n1n2[1]);

				/* Attendo la terminazione dei figli */
				waitpid(pid_n1, &status, 0);
				waitpid(pid_n2, &status, 0);

				/* Ricavo la grandezza del file */
				response_len = lseek(fd, 0, SEEK_END);
				
				/* Controllo aggiuntivo */
				if (response_len > UINT_MAX)
				{
					fprintf(stderr, "Errore. Output troppo grande!\n");
					exit(EXIT_FAILURE);
				}

				/* Codifico lunghezza della risposta da inviare */
				len[0] = (response_len & 0xFF00) >> 8;
				len[1] = (response_len & 0x00FF);

				/* Invio la lunghezza della risposta */
				if (write_all(ns, len, 2) < 0)
				{
					perror("write");
					exit(EXIT_FAILURE);
				}

				/* Invio la risposta */
				if (sendfile(ns, fd, 0, response_len) < 0)
				{
					perror("sendfile");
					exit(EXIT_FAILURE);
				}

				/* Rimuovo il file */
				close(fd);
				unlink(template);

				/* Proseguo con un ulteriore richiesta... */
			}

			/* Termino il figlio */
			exit(EXIT_SUCCESS);
		}

		/* PADRE */

		/* Chiudo la socket attiva */
		close(ns);
	}

	/* Chiudo la socket passiva */
	close(sd);

	/* Termino */
	return 0;
}