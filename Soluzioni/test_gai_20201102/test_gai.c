#include "dump.h"
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
        int ch, err;
        char *hostname;
        char *port;
        struct addrinfo hints, *res, *ptr;

        const char *progname = argv[0];

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;

        while ((ch = getopt(argc, argv, "sd46p")) != -1) {
                switch (ch) {
                        case 's':
                                hints.ai_socktype = SOCK_STREAM;
                                break;
                        case 'd':
                                hints.ai_socktype = SOCK_DGRAM;
                                break;
                        case '4':
                                hints.ai_family = AF_INET;
                                break;
                        case '6':
                                hints.ai_family = AF_INET6;
                                break;
                        case 'p':
                                hints.ai_flags = AI_PASSIVE;
                                break;
                }
        }
        argc -= optind;
        argv += optind;

        if (argc == 2) {
                hostname = argv[0];
                port = argv[1];
        } else if (argc == 1 && hints.ai_flags == AI_PASSIVE) {
                hostname = NULL;
                port = argv[0];
        } else {
                if (hints.ai_flags == AI_PASSIVE) {
                        fprintf(stderr, "Usage: %s -p [-s] [-d] [-6] [-4] hostname\n", progname);
                } else {
                        fprintf(stderr, "Usage: %s [-s] [-d] [-6] [-4] hostname port\n", progname);
                }
                exit(1);
        }

        if ((err = getaddrinfo(hostname, port, &hints, &res)) != 0) {
                fprintf(stderr, "Error in getaddrinfo: %s!\n", gai_strerror(errno));
                exit(2);
        }

        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
                puts("----------------------------------------");
                dumpaddrinfo(ptr, stdout);
                puts("----------------------------------------");
        }

        freeaddrinfo(res);

        return 0;
}

/*
 * vim: ts=8 sts=0 sw=8 expandtab
 */
