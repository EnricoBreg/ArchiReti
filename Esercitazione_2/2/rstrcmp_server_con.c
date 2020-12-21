#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

const unsigned int MAX_STRING_LENGTH = 100;
const unsigned int MAX_BUF_LENGTH = 2048;

// USAGE: ./rstrcmp_server numeroporta
int main(int argc, char const *argv[])
{
    int err, sd, req_num;
    struct addrinfo hints, *res;

    /* Controllo numero argomenti */
    if (argc != 2)
    {
        fprintf(stderr, "Errore. Uso: %s numeroporta\n", argv[0]);
        exit(-1);
    }

    /* Preparo le direttive per getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, argv[1], &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "Errore setup getaddrinfo: %s\n", gai_strerror(err));
        exit(-2);
    }

    /* Apro la socket */
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0)
    {
        perror("Errore funzione socket");
        close(sd);
        exit(-3);
    }

    /* Connetto la socket */
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Errore funzione bind");
        close(sd);
        exit(-4);
    }

    /* Trasformo la socket attiva in socket passiva */
    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("Errore funzione listen");
        close(sd);
        exit(-5);
    }

    printf("SERVER CONNESSO\n");

    for (req_num = 1;; req_num++)
    {
        int ns, nread, string1_length, string2_length, i, j, pid;
        char length_of_string1, length_of_string2;
        char buf[MAX_BUF_LENGTH], string1[MAX_STRING_LENGTH], string2[MAX_STRING_LENGTH];

        /* Accetto la richiesta */
        ns = accept(sd, NULL, NULL);
        if (ns < 0)
        {
            if (errno == EINTR)
            {
                perror("Errore funzione accept");
                close(sd);
                exit(-6);
            }
        }

        // Generazione del processo figlio
        pid = fork();
        if (pid < 0)
        {
            perror("Errore fork");
            continue;
        }

        if (pid == 0)
        {
            /* Codice processo figlio
            Chiusura socket passiva e proseguo con l'elaborazione della richiesta */
            close(sd);

            printf("\nRICHIESTA NUM. %d RICEVUTA, SERVITORE NUM. %d\n", req_num, getpid());

            /* Lettura della richiesta dell'utente */
            memset(buf, 0, sizeof(buf));
            nread = read(ns, buf, sizeof(buf));
            if (nread < 0)
            {
                perror("Errore funzione read");
                continue;
            }

            /* Estrapolazione delle stringhe 
            Preparo il necessario */
            memset(string1, 0, sizeof(string1));
            memset(string2, 0, sizeof(string2));
            memset(&length_of_string1, 0, sizeof(length_of_string1));
            memset(&length_of_string2, 0, sizeof(length_of_string2));
            string1_length = string2_length = 0;
            length_of_string1 = buf[0];
            string1_length = atoi(&length_of_string1);
            length_of_string2 = buf[string1_length + 1];
            string2_length = atoi(&length_of_string2);

            /* Prima stringa */
            printf("Lunghezza prima stringa: %c\n", length_of_string1);
            for (i = 1; i <= string1_length; i++)
            {
                string1[i - 1] = buf[i];
            }
            string1[string1_length] = '\0';
            printf("Prima stringa: %s\n", string1);

            /* Seconda stringa */
            printf("Lunghezza seconda stringa: %c\n", length_of_string2);
            i = 0;
            for (j = string1_length + 2, i = 0; i < string2_length; j++, i++)
            {
                string2[i] = buf[j];
            }
            string2[string2_length] = '\0';
            printf("Seconda stringa: %s\n", string2);

            /* Confronto ed invio della rispoosta al client */
            if (strcmp(string1, string2) == 0)
            {
                if (write(ns, "SI", 2) < 0)
                {
                    perror("Errore write server");
                    continue;
                }
            }
            else
            {
                if (write(ns, "NO", 2) < 0)
                {
                    perror("Errore write server");
                    continue;
                }
            }
            printf("\n");
            /* Chiusura della socket attiva */
            close(ns);
            /* Terminazione forzata del processo figlio */
            exit(-5);
        } //  Fine codice proc figlio

        /* Processo padre. Chiudo la socket attiva e proseguo con un'altra richiesta */
        close(ns);
    }

    /* Chiusura della socket passiva */
    close(sd);

    /* ANSI C main function returns 0*/
    return 0;
}
