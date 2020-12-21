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

const char file_path[] = "/home/enrico/dirprova/bollettino_neve/Veneto.txt";

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


int main(int argc, char const *argv[])
{
    int status, n_loc = 4;
    int p1p2[2], p2p0[2];
    char buffer[2028], *token, *ptr;
    long somma;
    ssize_t ret, nread;
    pid_t pid1, pid2, pid3;

    if (pipe(p1p2) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(p2p0) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if ((pid3 = fork()) < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid3 == 0) {
        // FIGLIO 3
        close(p1p2[0]);
        close(p1p2[1]);
        close(p2p0[0]);
        close(p2p0[1]);

        execlp("head", "head", "-n", "4", file_path, (char*)0);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    /* ==================== Calcolo media cm neve ======================*/
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // figlio 1 head
        close(p1p2[0]);
        close(p2p0[0]);
        close(p2p0[1]);

        // redirezione output
        close(1);
        dup(p1p2[1]);
        close(p1p2[1]);

        execlp("head", "head", "-n", "4", file_path, (char*)0);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    if ((pid2 = fork()) < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        // figlio 2 cut -n 1
        close(p1p2[1]);
        close(p2p0[0]);

        // redirezione input
        close(0);
        dup(p1p2[0]);
        close(p1p2[0]);

        // redirezione output
        close(1);
        dup(p2p0[1]);
        close(p2p0[1]);

        execlp("cut", "cut", "-f", "1", "-d", ",", (char*)0);
        perror("execlp");
        exit(EXIT_FAILURE);        
    }

    // PADRE

    close(p1p2[0]);
    close(p1p2[1]);
    close(p2p0[1]);
    
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
    
    if ((nread = read_all(p2p0[0], buffer, sizeof(buffer)) < 0)) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    token = strtok(buffer, "\n");
    while(token != NULL) {
        ret = strtol(token, &ptr, 10);
        somma += ret;  
        // token successivo
        token = strtok(NULL, "\n");
    }

    // stampa media
    printf("Media: %ld\n", somma / n_loc);

    return 0;
}
