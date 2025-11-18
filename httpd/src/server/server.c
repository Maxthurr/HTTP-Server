#define _GNU_SOURCE

#include "server.h"

#include <err.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../config/config.h"
#include "../utils/string/string.h"

static void handle_sigpipe(int sig)
{
    switch (sig)
    {
    case SIGPIPE:
        // TODO cleanup after closed connection
        printf("User disconnected (SIGPIPE received)\n");
        break;
    default:
        // Unsupported signal
        break;
    }
}

static struct addrinfo *get_ai(const struct server_config *config)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    const char *node = NULL;
    node = config->ip; // Set node to the server IP address

    struct addrinfo *result;
    int err = getaddrinfo(node, config->port, &hints, &result);
    if (err != 0)
        errx(EXIT_FAILURE, "%s", gai_strerror(err));

    return result;
}

static int create_socket(struct addrinfo *addr)
{
    int sfd;
    int e;

    struct addrinfo *p;
    for (p = addr; p; p = p->ai_next)
    {
        // If socket creation fails, try next address
        sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sfd == -1)
            continue;

        int opt = 1;
        // Reuse address when creating socket
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
        {
            freeaddrinfo(addr);
            err(EXIT_FAILURE, "setsockopt()");
        }

        // Bind socket to address
        e = bind(sfd, p->ai_addr, p->ai_addrlen);
        if (e != -1)
            break;

        close(sfd);
    }

    freeaddrinfo(addr);

    // If no socket could be created and bound, exit with error
    if (p == NULL)
        errx(EXIT_FAILURE, "Could not bind a socket");

    return sfd;
}

int start_server(const struct server_config *config)
{
    struct sigaction sa = { 0 };
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_sigpipe;

    // Ignore SIGPIPE to prevent server crash on client disconnect
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        err(EXIT_FAILURE, "sigaction");
    }

    // Open socket
    int sfd = create_socket(get_ai(config));
    return sfd;
}

void stop_server(int server_fd)
{
    close(server_fd);
}

int accept_connection(int sfd)
{
    int e = listen(sfd, 5);
    if (e == -1)
    {
        close(sfd);
        err(EXIT_FAILURE, "listen");
    }

    while (1)
    {
        printf("Waiting for connections...\n");
        int cfd = accept(sfd, NULL, NULL);
        if (cfd == -1)
            err(EXIT_FAILURE, "accept()");

        printf("Connection successful\n");

        ssize_t n;
        char buf[1024];
        while ((n = recv(cfd, buf, sizeof(buf) - 1, 0)) > 0)
        {
            struct string *string = string_create(buf, n + 1);
            string->data[n] =
                '\0'; //! This is only for debugging and should be removed later
            send(cfd, string->data, string->size, MSG_NOSIGNAL);
            printf("Received %zd bytes: %s. Sending back to client.\n",
                   string->size, string->data);
            string_destroy(string);
        }

        close(cfd);
    }

    stop_server(sfd);
    return 0;
}
