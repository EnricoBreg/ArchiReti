#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <error.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include "rxb.h"
#include "utils.h"

// Lunghezza massima del buffer di richiesta
const unsigned int MAX_REQUEST_SIZE = 64 * 1024;

void handler(int signo)
{
	int status;

	while (waitpid(-1, &status, WNOHANG) > 0)
	{
		continue;
	}
}

// Usage: ./rps_server porta
int main(int argc, char const *argv[])
{
	int err, sd, on, ns;
	struct addrinfo hints, *res;
	struct sigaction sa;
	pid_t pid1;
	// Controllo argomenti
	if (argc != 2)
	{
		fprintf(stderr, "Errore. Uso corretto: %s numero_porta\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Installo il gestore segnale SIGCHLD
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = handler;

	if (sigaction(SIGCHLD, &sa, NULL) < 0)
	{
		perror("Errore sigaction");
		exit(EXIT_FAILURE);
	}

	// preparazione getaddrinfo
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	// invocazione getaddrinfo
	err = getaddrinfo(NULL, argv[1], &hints, &res);
	if (err < 0)
	{
		fprintf(stderr, "Errore in getaddrinfo: %s\n", gai_strerror(err));
		exit(EXIT_FAILURE);
	}
	// Creazione della socket
	sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sd < 0)
	{
		perror("Errore socket");
		exit(EXIT_FAILURE);
	}
	// Disattivazione tempo timeout uscita socket
	on = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	{
		perror("Errore setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind della socket alla porta
	if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
	{
		perror("Errore bind");
		exit(EXIT_FAILURE);
	}
	// Deallocazione della memoria riservata a getaddrinfo
	freeaddrinfo(res);
	// Trasformo la socket attiva in passiva
	if (listen(sd, SOMAXCONN) < 0)
	{
		perror("Errore listes");
		exit(EXIT_FAILURE);
	}

	// Gestione delle richieste
	for (;;)
	{
		printf("Server connesso...\n");

		ns = accept(sd, NULL, NULL);
		if (ns < 0)
		{
			perror("Errore accept server");
			exit(EXIT_FAILURE);
		}

		// Generazione processo figlio
		pid1 = fork();
		if (pid1 < 0)
		{
			perror("Errore fork");
			exit(EXIT_FAILURE);
		}
		else if (pid1 == 0)
		{
			// Codice processo figlio per gestire le richieste
			int status;
			const char end_request[] = "\nFINE\n";
			rxb_t rxb_buffer;
			char option[MAX_REQUEST_SIZE];
			size_t option_len;
			pid_t pid2;

			// Figlio, Chiusura della socket
			close(sd);

			// Disinstallo il gestore del segnale installato nel padre
			memset(&sa, 0, sizeof(sa));
			sigemptyset(&sa.sa_mask);
			sa.sa_handler = SIG_DFL;

			if (sigaction(SIGCHLD, &sa, NULL) < 0)
			{
				perror("Errore sigaction figlio 1");
				exit(EXIT_FAILURE);
			}

			// Inizializzazione del buffer rxb
			rxb_init(&rxb_buffer, MAX_REQUEST_SIZE);

			// Avvio il ciclo di gestione delle richieste
			for (;;)
			{
				// Inizializzazione del buffer a null
				memset(option, 0, sizeof(option));
				option_len = sizeof(option) - 1;

				// Leggo le richieste dal client
				if (rxb_readline(&rxb_buffer, ns, option, &option_len) < 0)
				{
					// Se sono qui, il client ha chiuso la connessione
					fprintf(stderr, "Attenzione. Il client ha chiuso la connessione\n");
					rxb_destroy(&rxb_buffer);
					exit(EXIT_FAILURE);
				}

				// Figlio, creo un nipote per la l'output della richiesta al client
				pid2 = fork();
				if (pid2 < 0)
				{
					perror("Errore fork 2");
					exit(EXIT_FAILURE);
				}
				else if (pid2 == 0)
				{
					// Codice del processo figlio P2
					size_t opt_len;

					// Redirezione dell'output
					close(1);
					if (dup(ns) < 0)
					{
						perror("Errore dup");
						exit(EXIT_FAILURE);
					}
					close(ns);

					// Eseguo il ps corretto in base agli argomenti che il client ha inviato
					opt_len = strlen(option);
					if (opt_len == 0)
					{
						// Eseguo ps senza argomenti
						execlp("ps", "ps", (char *)0);
						perror("Errore execlp 1");
						exit(EXIT_FAILURE);
					}
					else
					{
						// Eseguo il ps con gli argomenti passati
						execlp("ps", "ps", option, (char *)0);
						perror("Errore execlp 2");
						exit(EXIT_SUCCESS);
					}
				}

				// Figlio 1, aspetto che il figlio (ps) termini
				wait(&status);

				// Mando la stringa di fine richiesta
				if (write_all(ns, end_request, strlen(end_request)) < 0)
				{
					perror("Errore write_all");
					exit(EXIT_FAILURE);
				}
			}

			// Chiusura della scocket
			close(ns);
			// Termino il figlio
			exit(EXIT_SUCCESS);
		}
		// Padre, chiudo la socket attiva
		close(ns);
	}
	// Chiudo la socket passiva
	close(sd);

	return 0;
}
