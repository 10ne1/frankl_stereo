/*
net.c                Copyright frankl 2013-2014

This file is part of frankl's stereo utilities. 
See the file License.txt of the distribution and 
http://www.gnu.org/licenses/gpl.txt for license details.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* returns file descriptor for network connection 
   (taken from man page of getaddrinfo)            */
int fd_net(char *host, char *port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(101);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }
    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect to %s:%s.\n", host, port);
        exit(102);
    }
    freeaddrinfo(result);           /* No longer needed */
    return sfd;
}

