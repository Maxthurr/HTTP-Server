#include "config.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/string/string.h"

enum opt
{
    PID_FILE,
    LOG_FILE,
    LOG,
    SERVER_NAME,
    PORT,
    IP,
    ROOT_DIR,
    DEFAULT_FILE,
    DAEMON
};

static enum daemon handle_daemon(struct config *config, char *arg)
{
    if (!strcmp("start", arg))
    {
        config->daemon = START;
        return true;
    }
    if (!strcmp("stop", arg))
    {
        config->daemon = STOP;
        return true;
    }
    if (!strcmp("restart", arg))
    {
        config->daemon = RESTART;
        return true;
    }

    return false;
}

static bool parse_options(int argc, char **argv, struct option *options,
                          struct config *config)
{
    int c;
    while ((c = getopt_long(argc, argv, "", options, NULL)) != -1)
    {
        switch (c)
        {
        case PID_FILE:
            config->pid_file = strdup(optarg);
            break;
        case LOG_FILE:
            config->log_file = strdup(optarg);
            break;
        case LOG:
            config->log = strcmp("true", optarg) == 0;
            break;
        case SERVER_NAME:
            config->servers->server_name =
                string_create(optarg, strlen(optarg));
            break;
        case PORT:
            config->servers->port = strdup(optarg);
            break;
        case IP:
            config->servers->ip = strdup(optarg);
            break;
        case ROOT_DIR:
            config->servers->root_dir = strdup(optarg);
            break;
        case DEFAULT_FILE:
            config->servers->default_file = strdup(optarg);
            break;
        case DAEMON:
            if (!handle_daemon(config, optarg))
                return false;
            break;
        default:
        case '?':
            return false;
            break;
        }
    }

    return true;
}

struct config *parse_configuration(int argc, char *argv[])
{
    struct option options[] = {
        { "pid_file", required_argument, NULL, PID_FILE },
        { "log_file", required_argument, NULL, LOG_FILE },
        { "log", required_argument, NULL, LOG },
        { "server_name", required_argument, NULL, SERVER_NAME },
        { "port", required_argument, NULL, PORT },
        { "ip", required_argument, NULL, IP },
        { "root_dir", required_argument, NULL, ROOT_DIR },
        { "default_file", required_argument, NULL, DEFAULT_FILE },
        { "daemon", required_argument, NULL, DAEMON },
    };

    struct config *config = calloc(1, sizeof(struct config));
    config->servers = calloc(1, sizeof(struct server_config));

    if (!parse_options(argc, argv, options, config) || !config->pid_file
        || !config->servers->server_name || !config->servers->port
        || !config->servers->ip || !config->servers->root_dir)
    {
        config_destroy(config);
        return NULL;
    }

    if (!config->servers->default_file)
        config->servers->default_file = strdup("index.html");
    if (config->daemon != NO_OPTION && !config->log_file)
        config->log_file = strdup("HTTPd.log");

    return config;
}

void config_destroy(struct config *config)
{
    free(config->pid_file);
    free(config->log_file);
    string_destroy(config->servers->server_name);
    free(config->servers->port);
    free(config->servers->ip);
    free(config->servers->root_dir);
    free(config->servers->default_file);
    free(config->servers);
    free(config);
}
