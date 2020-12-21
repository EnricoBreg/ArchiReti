#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

const char num_loc[] = "4";

ssize_t write_all(int fd, const void *vptr, size_t n)
{
    size_t remaining;
    ssize_t cc;
    const uint8_t *ptr;

    ptr = vptr;
    remaining = n;
    while (remaining > 0)
    {
        if ((cc = write(fd, ptr, remaining)) <= 0)
        {
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

int main(int argc, char const *argv[])
{
    int status;
    int p1p2[2], p2p3[2];
    char dir_files[] = "/home/enrico/dirprova/bollettino_neve/"; /* Directory contenente i file .txt delle località */
    char file_path[] = "/home/enrico/dirprova/bollettino_neve/Veneto.txt";
    char regione[100];
    pid_t pid1, pid2, pid3;

    /*
    scanf("%s", regione);

    sprintf(file_path, "%s%s.txt", dir_files, regione);

    puts(file_path);

    int fd = open(file_path, O_RDWR);
    if (fd > 0) {
        puts("Il file esiste");
    }
    else {
        puts("Il file non esiste");
    }
    close(fd);
    */

    if (pipe(p1p2) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(p2p3) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // head
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid1 == 0)
    {
        // Figlio 1
        close(p2p3[0]);
        close(p2p3[1]);
        close(p1p2[0]);

        // redirezione output
        close(1);
        dup(p1p2[1]);
        close(p1p2[1]);

        execlp("head", "head", "-n", num_loc, file_path, (char *)0);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    // sort
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid2 == 0)
    {
        // Figlio 2
        close(p1p2[1]);
        close(p2p3[0]);

        // redirezione input
        close(0);
        dup(p1p2[0]);
        close(p1p2[0]);

        // redirezione output
        close(1);
        dup(p2p3[1]);
        close(p2p3[1]);

        execlp("sort", "sort", "-n", (char *)0);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    // manipolazione output execlp (sort)
    pid3 = fork();
    if (pid3 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid3 == 0)
    {
        int nread, i = 0;
        char ch[1], *tok, *ptr;
        char buffer[1024], res_msg[1024];
        ssize_t ret, n_loc;
        long cm_neve_tot = 0;

        // Figlio 3
        // legge da pipe e calcola media

        // Chiudo descrittori non neccessari
        close(p1p2[0]);
        close(p1p2[1]);
        close(p2p3[1]);

        memset(buffer, 0, sizeof(buffer));
        while ((nread = read(p2p3[1], ch, strlen(ch)) > 0) && (ch != '\0'))
        {
            // controllo valore restituito da read
            if (nread < 0)
            {
                perror("read");
                exit(EXIT_FAILURE);
            }
            // Controllo che il carattere letto non sia un '\n'
            if (ch[0] == '\n')
            {
                /* Una volta finito di leggere il contenuto della pipe 
                 * uso strtok e sommo i valori di torno */
                tok = strtok(buffer, ",");
                // converto tok in long int con strtol, tok contiene solo il primo elemento
                // della lista ritornata da strtok cioè i cm di neve in quella località
                ret = strtol(tok, &ptr, 10);
                // Incremento i cm di neve totali
                cm_neve_tot += ret; // 
                // Scrivo sullo stdout il buffer
                printf("%s\n", buffer);
                if (write_all(1, buffer, strlen(buffer)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }

                // reset buffer
                i = 0;
                memset(buffer, 0, sizeof(buffer));
                // proseguo a leggere
                continue;
            }
            else
            {
                // Se diverso lo copio in un buffer temporaneo
                i++; // posizione i-esima del buffer dove copiare il carattere
                buffer[i] = *ch;
            }
        }
        /* Ora posso stampare la media dei cm di neve */
        n_loc = strtol(num_loc, &ptr, 10);
        sprintf(res_msg, "%s%ld\n", "La media dei cm di neve è: ", cm_neve_tot/n_loc);
        // scrivo sullo stdout
        if (write_all(1, res_msg, strlen(res_msg)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Termino
        exit(EXIT_SUCCESS);
    }

    // Padre
    close(p1p2[0]);
    close(p1p2[1]);
    close(p2p3[0]);
    close(p2p3[1]);

    wait(&status);
    wait(&status);
    wait(&status);

    return 0;
}
