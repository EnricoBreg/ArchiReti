#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
	int string_len;
	int len[2];
	char string[1024];

	/* HELLO GUYS = 10 caratteri/bytes */
	strcpy(string, "HELLO GUYS");

	string_len = strlen(string);
	/* Codifica della lunghezza */
	len[0] = (string_len & 0xFF00) >> 8;
	len[1] = (string_len & 0x00FF);

	/* Decodifica della lunghezza */
	int len_fin = len[0] << 8 | len[1];

	printf("La lunghezza della stringa è: %d\n", len_fin);

	return 0;
}
