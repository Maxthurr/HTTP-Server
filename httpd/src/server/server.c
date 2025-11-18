#define _GNU_SOURCE

#include "server.h"

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../config/config.h"
#include "../logger/logger.h"
#include "../utils/string/string.h"

static int sfd = -1;
static struct config *g_config = NULL;

static void handle_signals(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        // Graceful shutdown
        logger_log(g_config,
                   "-- Received termination signal, shutting down...\n");
        stop_server(sfd, g_config);
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
    {
        logger_log(g_config, gai_strerror(err));
        return NULL;
    }

    return result;
}

static int create_socket(struct addrinfo *addr)
{
    if (!addr)
        return -1;
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
            logger_log(g_config, strerror(errno));
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
    {
        logger_log(g_config, "-- Could not bind a socket");
        return -1;
    };

    return sfd;
}

int start_server(struct config *config)
{
    g_config = config;
    struct sigaction sa = { 0 };
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = SIG_IGN;

    // Ignore SIGPIPE to prevent server crash on client disconnect
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        err(EXIT_FAILURE, "sigaction");
    }

    // Handle SIGINT and SIGTERM for graceful shutdown
    sa.sa_flags = 0;
    sa.sa_handler = handle_signals;
    if (sigaction(SIGINT, &sa, NULL) == -1
        || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        logger_log(config, strerror(errno));
        return -1;
    }

    // Open socket
    sfd = create_socket(get_ai(config->servers));
    return sfd;
}

void stop_server(int server_fd, struct config *config)
{
    close(server_fd);
    logger_destroy();
    config_destroy(config);
}

int accept_connection(int sfd, struct config *config)
{
    int e = listen(sfd, 5);
    if (e == -1)
    {
        close(sfd);
        logger_log(config, strerror(errno));
        return 1;
    }

    while (1)
    {
        logger_log(config, "-- Waiting for connections...\n");
        int cfd = accept(sfd, NULL, NULL);
        if (cfd == -1)
        {
            close(sfd);
            logger_log(config, strerror(errno));
            return 1;
        }

        logger_log(config, "-- Connection successful\n");

        ssize_t n;
        char buf[1024];
        // Receive data from client
        while ((n = recv(cfd, buf, sizeof(buf) - 1, 0)) > 0)
        {
            struct string *string = string_create(buf, n + 1);
            string->data[n] =
                '\0'; //! This is only for debugging and should be removed later
            // Current behabior: echo back received data
            // TODO: Change to proper HTTP response
            send(cfd, string->data, string->size, MSG_NOSIGNAL);

            // Log received data
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Received %zd bytes: %s. Sending back to client.\n",
                     string->size, string->data);
            logger_log(config, msg);

            string_destroy(string);
        }

        close(cfd);
    }

    stop_server(sfd, config);
    return 0;
}
