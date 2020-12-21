#include <stdio.h>
#include <string.h>


int main(int argc, char const *argv[])
{
    char stringa[10];

    memset(stringa, 0, sizeof(stringa));
    if (stringa[0] == '\0') {
        printf("Stringa nulla\n");
    }
    else {
        printf("La stringa contiene qualcosa\n%s\n", stringa);
    }

    return 0;
}
