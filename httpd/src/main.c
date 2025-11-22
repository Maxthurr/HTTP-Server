#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "daemon/daemon.h"
#include "logger/logger.h"
#include "server/server.h"
#include "utils/string/string.h"

void print_server_config(struct config *config)
{
    char msg[512];

    if (config)
    {
        if (config->servers->server_name)
        {
            char *name =
                calloc(config->servers->server_name->size + 1, sizeof(char));
            memcpy(name, config->servers->server_name->data,
                   config->servers->server_name->size);

            sprintf(msg, "Server Name: %s", name);
            logger_log(config, msg);
            free(name);
        }
        if (config->servers->port)
        {
            sprintf(msg, "Port: %s", config->servers->port);
            logger_log(config, msg);
        }
        else
            logger_log(config, "Port: (not set)");
        if (config->servers->ip)
        {
            sprintf(msg, "IP: %s", config->servers->ip);
            logger_log(config, msg);
        }
        else
            logger_log(config, "IP: (not set)");
        if (config->servers->root_dir)
        {
            sprintf(msg, "Root Directory: %s", config->servers->root_dir);
            logger_log(config, msg);
        }
        else
            logger_log(config, "Root Directory: (not set)");
        if (config->servers->default_file)
        {
            sprintf(msg, "Default File: %s", config->servers->default_file);
            logger_log(config, msg);
        }
        else
            logger_log(config, "Default File: (not set)");
    }
}

void print_config(struct config *config)
{
    char msg[512];

    if (config->pid_file)
    {
        sprintf(msg, "PID File: %s", config->pid_file);
        logger_log(config, msg);
    }
    else
        logger_log(config, "PID File: (not set)");
    if (config->log_file)
    {
        sprintf(msg, "Log File: %s", config->log_file);
        logger_log(config, msg);
    }
    else
        logger_log(config, "Log File: (not set)");

    sprintf(msg, "Log Enabled: %s", config->log ? "true" : "false");
    logger_log(config, msg);

    print_server_config(config);

    switch (config->daemon)
    {
    case NO_OPTION:
        logger_log(config, "Daemon Option: NO_OPTION");
        break;
    case START:
        logger_log(config, "Daemon Option: START");
        break;
    case STOP:
        logger_log(config, "Daemon Option: STOP");
        break;
    case RESTART:
        logger_log(config, "Daemon Option: RESTART");
        break;
    default:
        logger_log(config, "Daemon Option: UNKNOWN");
        break;
    }
}

int main(int argc, char **argv)
{
    if (!argc)
        return 1;

    struct config *config = parse_configuration(argc, argv);

    switch (config->daemon)
    {
    case START:
        if (start_daemon(config))
            return 1;
        break;
    case STOP:
        stop_daemon(config);
        config_destroy(config);
        return 0;
    case RESTART:
        restart_daemon(config);
        break;
    case NO_OPTION:
    default:
        break;
    }

    logger_init(config);
    logger_log(config, "-- Configuration Parsed");
    print_config(config);

    logger_log(config, "-- Starting server...");
    int server_fd = start_server(config);

    if (server_fd != -1)
    {
        logger_log(config, "-- Server started.");
        int e = run_server(server_fd, config);
        if (e == -1)
            logger_log(config, "-- Error accepting connections.");

        return 0;
    }

    return 1;
}
