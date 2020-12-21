#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned int BUF_MAX_SIZE = 100;
const unsigned int MAX_SIZE = 11;

int main(int argc, char const *argv[])
{
    int i, j, string1_length, string2_length;
    char length_of_string1, length_of_string2;
    //char buf[] = "4ciao4ciao";
    char buf[BUF_MAX_SIZE];
    char string1[MAX_SIZE], string2[MAX_SIZE];

    sprintf(buf, "%d%s%d%s", 4, "ciao", 4, "ciao");

    /*for(i = 0; i < MAX_SIZE; i++) {
        printf("%d\t%c\n", i, buf[i]);
    }*/

    length_of_string1 = buf[0];
    string1_length = atoi(&length_of_string1);
    length_of_string2 = buf[string1_length + 1];
    string2_length = atoi(&length_of_string2);

    /*printf("%c\n", length_of_string1);
    printf("%d\n", string1_length);
    printf("%c\n", length_of_string2);
    printf("%d\n", string2_length);*/

    memset(string1, 0, sizeof(string1));
    memset(string2, 0, sizeof(string2));

    for (i = 1; i <= string1_length; i++)
    {
        string1[i - 1] = buf[i];
    }
    string1[string1_length] = '\0';
    printf("%s\n", string1);

    i = 0;
    for (j = string1_length + 2, i=0; i < string2_length; j++, i++)
    {
        string2[i] = buf[j];
    }
    string2[string2_length] = '\0';
    printf("%s\n", string2);

    if(strcmp(string1, string2) == 0) {
        printf("Le stringhe sono uguali\n");
    }
    else{
        printf("Le stringhe sono diverse\n");
    }

    return 0;
}
