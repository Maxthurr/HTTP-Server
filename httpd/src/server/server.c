#define _GNU_SOURCE

#include "server.h"

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../config/config.h"
#include "../file/file.h"
#include "../http/http.h"
#include "../logger/logger.h"
#include "../utils/string/string.h"

static int sfd = -1;
static struct config *g_config = NULL;
static int cfd = -1;

static void handle_signals(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        // Graceful shutdown
        logger_log(g_config,
                   "-- Received termination signal, shutting down...");
        stop_server(cfd, sfd, g_config);
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
        logger_error(g_config, "getaddrinfo()", gai_strerror(err));
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
            logger_error(g_config, "setsockopt()", strerror(errno));
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
        logger_error(config, "sigaction()", strerror(errno));
        return -1;
    }

    // Handle SIGINT and SIGTERM for graceful shutdown
    sa.sa_flags = 0;
    sa.sa_handler = handle_signals;
    if (sigaction(SIGINT, &sa, NULL) == -1
        || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        logger_error(config, "sigaction()", strerror(errno));
        return -1;
    }

    // Open socket
    sfd = create_socket(get_ai(config->servers));
    return sfd;
}

void stop_server(int cfd, int server_fd, struct config *config)
{
    if (cfd != -1)
        close(cfd);

    close(server_fd);
    logger_destroy();
    config_destroy(config);
}

static void send_data(struct config *config, int fd,
                      struct response_header *response)
{
    struct string *response_str = response_header_to_string(response);

    size_t total_sent = 0;
    // Send response header
    while (total_sent < response_str->size)
    {
        ssize_t sent = send(cfd, response_str->data + total_sent,
                            response_str->size - total_sent, MSG_NOSIGNAL);
        if (sent == -1)
        {
            logger_error(config, "send()", strerror(errno));
            break;
        }

        total_sent += sent;
    }

    if (fd != -1)
    {
        off_t file_sent = 0;
        // Send file content
        while (file_sent < response->content_length)
        {
            ssize_t sent =
                sendfile(cfd, fd, NULL, response->content_length - file_sent);
            if (sent == -1)
            {
                logger_error(config, "sendfile()", strerror(errno));
                break;
            }

            file_sent += sent;
        }
    }

    string_destroy(response_str);
}

static void handle_request(struct config *config, struct string *request)
// struct string *sender)
{
    struct request_header *req_header = parse_request(request);

    off_t content_length = 0;
    int fd = -1;

    if (req_header->status == OK)
    {
        content_length = get_file_length(req_header->filename->data);
        if (content_length == -1)
        {
            req_header->status = NOT_FOUND;
            content_length = 0;
        }
        else if (req_header->method == GET)
        {
            fd = open(req_header->filename->data, O_RDONLY);
            if (fd == -1)
            {
                req_header->status = FORBIDDEN;
                content_length = 0;
            }
        }
    }

    struct response_header *response =
        create_response(req_header, content_length);

    send_data(config, fd, response);

    destroy_request(req_header);
    destroy_response(response);
    if (fd != -1)
        close(fd);
}

int accept_connection(int sfd, struct config *config)
{
    int e = listen(sfd, 5);
    if (e == -1)
    {
        close(sfd);
        logger_error(config, "listen()", strerror(errno));
        return 1;
    }

    while (1)
    {
        logger_log(config, "-- Waiting for connections...");

        struct sockaddr addr = { 0 };
        socklen_t addr_len = sizeof(addr);

        cfd = accept(sfd, &addr, &addr_len);
        if (cfd == -1)
        {
            close(sfd);
            logger_error(config, "accept()", strerror(errno));
            return 1;
        }

        void *tmp = &addr;
        struct sockaddr_in *in_addr = tmp;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &in_addr->sin_addr, ip_str, sizeof(ip_str));
        struct string *sender = string_create(ip_str, strlen(ip_str));

        logger_log(config, "-- Connection successful");

        ssize_t n;
        char buf[1024];

        struct string *request = string_create("", 0);

        // Receive data from client
        while ((n = recv(cfd, buf, sizeof(buf) - 1, 0)) > 0)
        {
            string_concat_str(request, buf, n);
            // Check for end of HTTP header
            if (memmem(request->data, request->size, "\r\n\r\n", 4))
                break;
        }

        logger_log(config, "-- Request received");

        handle_request(config, request); //, sender);
        string_destroy(sender);
        string_destroy(request);

        close(cfd);
        cfd = -1;
    }

    stop_server(cfd, sfd, config);
    return 0;
}
