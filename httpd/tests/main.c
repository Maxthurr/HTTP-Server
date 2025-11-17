#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/config/config.h"
#include "../src/utils/string/string.h"

int main(int argc, char **argv)
{
    if (!argc)
        return 1;

    struct config *config = parse_configuration(argc, argv);
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
        config_destroy(config);
    }
    else
    {
        printf("Failed to parse configuration.\n");
    }

    return 0;
}
