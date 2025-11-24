#define _GNU_SOURCE

#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../config/config.h"
#include "../http/http.h"
#include "../logger/logger.h"
#include "../utils/file/file.h"
#include "../utils/string/string.h"

#define MAX_EVENTS 64

static struct config *g_config = NULL;
static volatile sig_atomic_t shutdown_needed = false;

struct connection
{
    int fd;
    struct string *sender;
    struct string *request;
};

static void handle_signals(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        // Graceful shutdown
        logger_log(g_config,
                   "-- Received termination signal, shutting down...");
        shutdown_needed = true;
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

static int set_nonblocking(int fd)
{
    // Get current flags
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;

    // Add non blocking flag
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int create_socket(struct addrinfo *addr)
{
    if (!addr)
        return -1;
    int e;

    int sfd = -1;
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

        // Could not bind, close current socket and try next address
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
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;

    // Ignore SIGPIPE to prevent server crash on client disconnect
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        logger_error(config, "sigaction()", strerror(errno));
        return -1;
    }

    // Handle SIGINT and SIGTERM for graceful shutdown
    sa.sa_handler = handle_signals;
    if (sigaction(SIGINT, &sa, NULL) == -1
        || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        logger_error(config, "sigaction()", strerror(errno));
        return -1;
    }

    // Open socket
    int sfd = create_socket(get_ai(config->servers));
    return sfd;
}

void stop_server(int server_fd, struct config *config)
{
    close(server_fd);
    logger_destroy();
    config_destroy(config);
}

static void send_data(const struct config *config, int cfd, int fd,
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

    // Send file content if needed
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

static void handle_request(const struct config *config, struct string *request,
                           struct string *sender, int cfd)
{
    struct request_header *req_header = parse_request(request, config);
    logger_request(config, req_header, sender);

    // Get full file path from server root_directory
    struct string *full_filename = get_fullname(config, req_header->filename);
    off_t content_length = 0;

    int fd = -1;
    if (req_header->status == OK)
    {
        content_length = get_file_length(full_filename->data);
        if (content_length == -1)
        {
            req_header->status = NOT_FOUND;
            content_length = 0;
        }
        else if (req_header->method == GET)
        {
            fd = open(full_filename->data, O_RDONLY);
            if (fd == -1)
            {
                req_header->status = FORBIDDEN;
                content_length = 0;
            }
        }
    }

    string_destroy(full_filename);

    struct response_header *response =
        create_response(req_header, content_length);
    logger_response(config, req_header, sender);

    // Answer client's request
    send_data(config, cfd, fd, response);

    // Clean up
    destroy_request(req_header);
    destroy_response(response);
    if (fd != -1)
        close(fd);
}

static struct connection *create_connection(int fd, struct string *sender)
{
    struct connection *connection = calloc(1, sizeof(struct connection));

    if (!connection)
        return NULL;

    connection->fd = fd;
    connection->sender = sender;
    connection->request = string_create("", 0);
    return connection;
}

static void free_connection(struct connection *connection)
{
    if (!connection)
        return;

    string_destroy(connection->sender);
    string_destroy(connection->request);

    if (connection->fd != -1)
        close(connection->fd);

    free(connection);
}

static int receive_client_data(const struct config *config,
                               struct connection *connection)
{
    ssize_t n;
    char buf[1024];

    while (!shutdown_needed)
    {
        n = recv(connection->fd, buf, sizeof(buf) - 1, 0);

        // Data received
        if (n > 0)
        {
            string_concat_str(connection->request, buf, n);

            // Header fully received
            if (memmem(connection->request->data, connection->request->size,
                       "\r\n\r\n", 4))
                return 1;

            continue;
        }

        // No data to receive from client anymore
        if (n == 0)
            return -1;

        if (n == -1)
        {
            // Error
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;

            // Interrupt signal received
            if (errno == EINTR)
                continue;

            logger_error(config, "recv()", strerror(errno));
            return -1;
        }
    }

    return 0;
}

static void accept_and_register(int epfd, int sfd, struct config *config)
{
    while (!shutdown_needed)
    {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int cfd = accept(sfd, &addr, &addr_len);
        if (cfd == -1)
        {
            // No more incoming connections to accept
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            // Interrupted by signal
            if (errno == EINTR)
                continue;

            logger_error(config, "accept()", strerror(errno));
            break;
        }

        // Set connection to non blocking
        if (set_nonblocking(cfd) == -1)
        {
            close(cfd);
            continue;
        }

        // Get client IPv4 address
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
        struct string *sender = string_create(ip_str, strlen(ip_str) + 1);

        struct connection *connection = create_connection(cfd, sender);
        if (!connection)
        {
            string_destroy(sender);
            close(cfd);
            continue;
        }

        struct epoll_event conn_event;
        conn_event.events = EPOLLIN | EPOLLRDHUP;
        conn_event.data.ptr = connection;

        // Register new connection
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &conn_event) == -1)
        {
            free_connection(connection);
            continue;
        }
    }
}

static int setup_epoll(int sfd, struct config *config)
{
    // Start listening
    if (listen(sfd, SOMAXCONN))
    {
        close(sfd);
        logger_error(config, "listen()", strerror(errno));
        return 1;
    }

    // Set listening socket to non blocking
    if (set_nonblocking(sfd) == -1)
    {
        close(sfd);
        logger_error(config, "set_nonblocking()", strerror(errno));
        return 1;
    }

    // Create epoll instance
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        logger_error(config, "epoll_create1()", strerror(errno));
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;

    // Register listening socket
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        logger_error(config, "epoll_ctl ADD listen", strerror(errno));
        close(epfd);
        return 1;
    }

    return epfd;
}

static void close_connection(int epfd, struct connection *connection)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, connection->fd, NULL);
    free_connection(connection);
}

int run_server(int sfd, struct config *config)
{
    int epfd = setup_epoll(sfd, config);
    if (epfd == -1)
        return 1;

    struct epoll_event events[MAX_EVENTS];
    while (!shutdown_needed)
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1)
        {
            // If interrupted by a signal, go to next iteration
            if (errno == EINTR)
                continue;

            logger_error(config, "epoll_wait()", strerror(errno));
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            struct epoll_event *event = &events[i];

            // Register incoming connection
            if (event->data.ptr == NULL)
            {
                accept_and_register(epfd, sfd, config);
                continue;
            }

            struct connection *connection = event->data.ptr;
            // Error occured in event
            if (event->events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                close_connection(epfd, connection);
                continue;
            }

            // Process received data
            if (event->events & EPOLLIN)
            {
                int received = receive_client_data(config, connection);
                if (received == 1)
                {
                    // Full request received
                    handle_request(config, connection->request,
                                   connection->sender, connection->fd);
                }

                // Close connection after handling request
                close_connection(epfd, connection);
            }
        }
    }

    close(epfd);
    stop_server(sfd, config);
    return 0;
}
