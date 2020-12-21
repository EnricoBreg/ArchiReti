#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
#include <unistd.h>
#include "rxb.h"
#include "utils.h"

typedef enum {
        START_MESSAGE,
        READ_LENGTH_START,
        READ_LENGTH_CONTINUE
} parser_state_t;

#define MAX_TOKEN_SIZE 4096

int simple_csexp_read_message(struct rx_buffer *rxb, int fd, char **out, size_t *out_len)
{
        parser_state_t ps = START_MESSAGE;
        unsigned int string_index = 0;
        int next_string_length = -1;
        char *c;
        char current_token[MAX_TOKEN_SIZE] = {0};
        char *d = current_token;
        size_t bytes_left;
	int ret;

        /* argument check */
        if (out == NULL || out_len == NULL || *out_len == 0) {
                return -1;
        }

        /* read some data */
        if (rxb_available(rxb) == 0) {
                int cc = rxb_read_bytes(rxb, fd);
                if (cc <= 0) {
                        /* I/O error or EOF */
                        return -2;
                }
        }
        /* initialize c and bytes_left */
        c = (char *)rxb_peek(rxb);
        bytes_left = rxb_available(rxb);

        while (1) {
                /* read more data if needed */
                if (bytes_left == 0) {
                        int cc = rxb_read_bytes(rxb, fd);
                        if (cc <= 0) {
                                /* I/O error or EOF */
                                ret = -3;
                                goto CLEANUP;
                        }
                        /* reinitialize c and bytes_left */
                        c = (char *)rxb_peek(rxb);
                        bytes_left = rxb_available(rxb);
                }

                switch(ps) {
                        case START_MESSAGE:
                                printf("START_MESSAGE w/ *c == '%c'\n", *c);
                                if (*c != '(') {
                                        ret = -4;
                                        goto CLEANUP;
                                }

                                ps = READ_LENGTH_START;
                                break;

                        case READ_LENGTH_START:
                                printf("READ_LENGTH_START w/ *c == '%c'\n", *c);
                                if (*c == ')') {
                                        /* total valid bytes in rxb */
                                        size_t available = rxb_available(rxb);
					printf("available: %ld\n", available);

                                        /* bytes remaining to process in rxb */
                                        size_t unprocessed = bytes_left - 1;
					printf("unprocessed: %ld\n", unprocessed);

                                        /* drop the bytes in the buffer that we already used */
                                        rxb_drop_bytes(rxb, available - unprocessed);

                                        /* set *out_len */
                                        *out_len = string_index;

                                        /* we finished reading the message;
                                         * let's return the number of
                                         * unprocessed characters left in the
                                         * buffer */
                                        return unprocessed;
                                } else if (*c < '1' || *c > '9') { /* first digit can't be 0 */
                                        /* not a digit */
                                        ret = -5;
                                        goto CLEANUP;
                                }

                                /* make sure we are not overrunning out buffer */
                                if (string_index >= *out_len) {
                                        ret = -6;
                                        goto CLEANUP;
                                }

                                *d++ = *c;
                                ps = READ_LENGTH_CONTINUE;
                                break;

                        case READ_LENGTH_CONTINUE:
                                printf("READ_LENGTH_CONTINUE w/ *c == '%c'\n", *c);
                                if (*c == ':') {
                                        /* get next string length */
                                        next_string_length = atoi(current_token);
                                        printf("READ_LENGTH_CONTINUE w/ next_string_length == '%d'\n", next_string_length);

                                        /* extract string */
                                        char *string = strndup(c+1, next_string_length);
                                        if (string == NULL) {
                                                ret = -7;
                                                goto CLEANUP;
                                        }
                                        out[string_index++] = string;

                                        /* update next char pointer and bytes_left */
                                        c += next_string_length;
                                        bytes_left -= next_string_length;

                                        /* reset current_token */
                                        memset(current_token, 0, sizeof(current_token));
                                        d = current_token;

                                        ps = READ_LENGTH_START;
                                        break;
                                } else if (*c < '0' || *c > '9') {
                                        /* not a digit */
                                        ret = -8;
                                        goto CLEANUP;
                                }

                                *d++ = *c;
                                ps = READ_LENGTH_CONTINUE;
                                break;

                        default:
                                /* unrecognized state */
                                ret = -9;
                                goto CLEANUP;
                }
                ++c; --bytes_left;
        }

CLEANUP:
        /* free all allocated strings */
        while (string_index > 0) {
                free(out[--string_index]);
        }
        *out_len = 0;
        return ret;
}


/* helper function for simple_csexp_print_message */
static int print_bytes_from_fd(struct rx_buffer *rxb, int fd, size_t to_write) 
{
        int cc;
        size_t available;

        while (to_write > 0) {
                cc = rxb_read_bytes(rxb, fd);
                if (cc <= 0) {
                        /* I/O error or EOF */
                        return -1;
                }

                available = rxb_available(rxb);
                if (to_write > available) {
                        cc = write_all(1, rxb_peek(rxb), available);
                        if (cc <= 0) {
                                /* I/O error or EOF */
                                return -2;
                        }
                        to_write -= available;
                } else {
                        cc = write_all(1, rxb_peek(rxb), to_write);
                        if (cc <= 0) {
                                /* I/O error or EOF */
                                return -2;
                        }
                        rxb_drop_bytes(rxb, to_write);
                        to_write = 0;
                }
        }

        return 0;
}


int simple_csexp_print_message(struct rx_buffer *rxb, int fd)
{
        parser_state_t ps = START_MESSAGE;
        unsigned int string_number = 1;
        int next_string_length;
        char current_token[MAX_TOKEN_SIZE] = {0};
        char *c, *d = current_token;
        size_t bytes_left;

        /* read some data */
        if (rxb_available(rxb) == 0) {
                int cc = rxb_read_bytes(rxb, fd);
                if (cc <= 0) {
                        /* I/O error or EOF */
                        return -1;
                }
        }

        /* initialize c and bytes_left */
        c = (char *)rxb_peek(rxb);
        bytes_left = rxb_available(rxb);

        while (1) {
                /* read more data if needed */
                if (bytes_left == 0) {
                        int cc = rxb_read_bytes(rxb, fd);
                        if (cc <= 0) {
                                /* I/O error or EOF */
                                return -2;
                        }

                        /* reinitialize c and bytes_left */
                        c = (char *)rxb_peek(rxb);
                        bytes_left = rxb_available(rxb);
                }

                switch(ps) {
                        case START_MESSAGE:
                                /* printf("START_MESSAGE w/ *c == '%c'\n", *c); */
                                if (*c != '(') {
                                        return -3;
                                }

                                ps = READ_LENGTH_START;
                                break;

                        case READ_LENGTH_START:
                                /* printf("READ_LENGTH_START w/ *c == '%c'\n", *c); */
                                if (*c == ')') {
                                        /* total valid bytes in rxb */
                                        size_t available = rxb_available(rxb);

                                        /* bytes remaining to process in rxb */
                                        size_t unprocessed = bytes_left - 1;

                                        /* drop the bytes in the buffer that we already used */
                                        rxb_drop_bytes(rxb, available - unprocessed);

                                        /* we finished reading the message;
                                         * let's return the number of
                                         * unprocessed characters left in the
                                         * buffer */
                                        return unprocessed;
                                } else if (*c < '1' || *c > '9') { /* first digit can't be 0 */
                                        /* not a digit */
                                        return -4;
                                }

                                *d++ = *c;
                                ps = READ_LENGTH_CONTINUE;
                                break;

                        case READ_LENGTH_CONTINUE:
                                /* printf("READ_LENGTH_CONTINUE w/ *c == '%c'\n", *c); */
                                if (*c == ':') {
                                        /* get next string length */
                                        next_string_length = atoi(current_token);
                                        /* printf("READ_LENGTH_CONTINUE w/ next_string_length == '%d'\n", next_string_length); */

                                        /* print string */
                                        printf("String number %d:\n", string_number++); 
					fflush(stdout);

                                        /* if we can serve all from the buffer */
                                        if (next_string_length < bytes_left - 1) {
                                                write_all(1, c+1, next_string_length);

                                                /* update next char pointer and bytes_left */
                                                c += next_string_length;
                                                bytes_left -= next_string_length;
                                        } else {
                                                size_t to_write = next_string_length;
                                                write_all(1, c+1, bytes_left-1);
                                                to_write -= (bytes_left - 1);

                                                print_bytes_from_fd(rxb, fd, to_write);

                                                /* reinitialize c and bytes_left */
                                                c = (char *)rxb_peek(rxb);
                                                bytes_left = rxb_available(rxb);
                                        }

                                        printf("\n\n"); 

                                        /* reset current_token */
                                        memset(current_token, 0, sizeof(current_token));
                                        d = current_token;

                                        ps = READ_LENGTH_START;
                                        break;
                                } else if (*c < '0' || *c > '9') {
                                        /* not a digit */
                                        return -5;
                                }

                                *d++ = *c;
                                ps = READ_LENGTH_CONTINUE;
                                break;

                        default:
                                /* unrecognized state */
                                return -6;
                }
                ++c; --bytes_left;
        }

	/* control flow should never arrive here: if it does, we have a problem */
        return -7;
}