#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

const char num_loc[] = "4";

int main(int argc, char const *argv[])
{
    char *tok, *ptr;
    char stringa[] = "120,Cortina d'Ampezzo,SnowLife120,Falcade,SnowLife2150,Arabba,SnowLife3170,Alleghe,SnowLife4";
    long ret, tot_cm_neve;

    tok = strtok(stringa, ",");
    while (tok != NULL)
    {
        ret = strtol(tok, &ptr, 10);
        tot_cm_neve += ret;
        printf("%ld\n", ret);

        tok = strtok(NULL, ","); // token successivo
    }

    int numero_loc = strtol(num_loc, &ptr, 10);
    printf("\nMedia cm di neve: %ld\n", tot_cm_neve / numero_loc);

    char prova[] = "tiamo3000";
    long int test = strtol(prova, &ptr, 10);
    printf("%ld\n", test);

    return 0;
}
