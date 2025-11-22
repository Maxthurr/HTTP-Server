#include "logger.h"

#include <stdio.h>
#include <time.h>

#include "../config/config.h"
#include "../http/http.h"
#include "../utils/string/string.h"

static FILE *log_file = NULL;

int logger_init(const struct config *config)
{
    if (!config->log)
        return 0;

    if (!config->log_file)
    {
        log_file = stdout;
        return 0;
    }

    log_file = fopen(config->log_file, "w");
    if (!log_file)
        return 1;

    return 0;
}

void logger_log(const struct config *config, const char *message)
{
    if (!log_file || !config->log)
        return;

    // Get server name
    char server_name[256];
    snprintf(server_name, config->servers->server_name->size + 1, "%s",
             config->servers->server_name->data);
    // Get current time in GMT
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    // Format time as Day (3 letters), DD Mon YYYY HH:MM:SS GMT
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    fprintf(log_file, "%s [%s] %s\n", time_str, server_name, message);
}

static const char *status_to_string(enum request_status status)
{
    switch (status)
    {
    case BAD_REQUEST:
        return "Bad Request";
    case FORBIDDEN:
        return "Forbidden";
    case NOT_FOUND:
        return "Not Found";
    case METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case UNSUPPORTED_VERSION:
        return "HTTP Version Not Supported";
    default:
        return "Internal Server Error";
    }
}

void logger_request(const struct config *config,
                    const struct request_header *request,
                    const struct string *client_ip)
{
    if (!log_file || !config->log)
        return;

    char msg[512];
    if (request->status == OK)
    {
        sprintf(msg, "received %s on '%s' from %s",
                request->method == GET ? "GET" : "HEAD",
                request->filename ? request->filename->data : "/",
                client_ip->data);
    }
    else
    {
        sprintf(msg, "received %s from %s", status_to_string(request->status),
                client_ip->data);
    }
    logger_log(config, msg);
}

void logger_response(const struct config *config,
                     const struct request_header *request,
                     const struct string *client_ip)
{
    if (!log_file || !config->log)
        return;

    char msg[512];

    if (request->status == BAD_REQUEST)
    {
        sprintf(msg, "responded with %d to %s", request->status,
                client_ip->data);
    }
    else
    {
        char *method = request->method == GET ? "GET"
            : request->method == HEAD         ? "HEAD"
                                              : "UNKNOWN";

        sprintf(msg, "responded with %d to %s for %s on '%s'", request->status,
                client_ip->data, method,
                request->filename ? request->filename->data : "/");
    }

    logger_log(config, msg);
}

void logger_error(const struct config *config, const char *source,
                  const char *message)
{
    if (!log_file || !config->log)
        return;

    char msg[512];
    sprintf(msg, "An error occured in %s: %s", source, message);
    logger_log(config, msg);
}

void logger_destroy(void)
{
    // Ensure all logs are flushed before closing
    fflush(log_file);
    if (log_file && log_file != stdout)
        fclose(log_file);
    log_file = NULL;
}
