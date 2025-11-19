#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/daemon/daemon.h"
#include "../src/logger/logger.h"
#include "../src/server/server.h"
#include "../src/utils/string/string.h"

void print_config(struct config *config)
{
    if (config)
    {
        if (config->pid_file)
            printf("PID File: %s\n", config->pid_file);
        else
            printf("PID File: (not set)\n");
        if (config->log_file)
            printf("Log File: %s\n", config->log_file);
        else
            printf("Log File: (not set)\n");
        printf("Logging Enabled: %s\n", config->log ? "true" : "false");
        if (config->servers)
        {
            if (config->servers->server_name)
            {
                char *name = calloc(config->servers->server_name->size + 1,
                                    sizeof(char));
                memcpy(name, config->servers->server_name->data,
                       config->servers->server_name->size);
                printf("Server Name: %s\n", name);
                free(name);
            }
            if (config->servers->port)
                printf("Port: %s\n", config->servers->port);
            else
                printf("Port: (not set)\n");
            if (config->servers->ip)
                printf("IP: %s\n", config->servers->ip);
            else
                printf("IP: (not set)\n");
            if (config->servers->root_dir)
                printf("Root Directory: %s\n", config->servers->root_dir);
            else
                printf("Root Directory: (not set)\n");
            if (config->servers->default_file)
                printf("Default File: %s\n", config->servers->default_file);
            else
                printf("Default File: (not set)\n");
        }
        switch (config->daemon)
        {
        case NO_OPTION:
            printf("Daemon Option: NO_OPTION\n");
            break;
        case START:
            printf("Daemon Option: START\n");
            break;
        case STOP:
            printf("Daemon Option: STOP\n");
            break;
        case RESTART:
            printf("Daemon Option: RESTART\n");
            break;
        default:
            printf("Daemon Option: UNKNOWN\n");
            break;
        }
    }
    else
    {
        printf("Failed to parse configuration.\n");
    }
}

int main(int argc, char **argv)
{
    if (!argc)
        return 1;

    struct config *config = parse_configuration(argc, argv);
    print_config(config);

    logger_init(config);

    switch (config->daemon)
    {
    case START:
        printf("\n-- Starting daemon\n");
        if (start_daemon(config))
            return 1;
        break;
    case STOP:
        printf("\n-- Stopping daemon\n");
        stop_daemon(config);
        logger_destroy();
        config_destroy(config);
        return 0;
    case RESTART:
        printf("\n-- Restarting daemon\n");
        restart_daemon(config);
        break;
    case NO_OPTION:
    default:
        printf("\n-- No daemon option specified.");
        break;
    }

    logger_log(config, "-- Starting server...");
    int server_fd = start_server(config);

    if (server_fd != -1)
    {
        logger_log(config, "-- Server started.");
        int e = accept_connection(server_fd, config);
        if (e == -1)
            logger_log(config, "-- Error accepting connections.");

        return 0;
    }

    return 1;
}
