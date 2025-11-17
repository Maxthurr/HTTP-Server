#ifndef CONFIG_H
#define CONFIG_H

#define _XOPEN_SOURCE 500

#include <stdbool.h>

/*
** @brief Enum daemon
** NO_OPTION if the '--daemon' option is not given
** START, STOP, RESTART if option is "start", "stop" and "restart"
*/
enum daemon
{
    NO_OPTION = 0,
    START,
    STOP,
    RESTART
};

/*
** @brief Configuration structure
**
** @param pid_file Path to the pid file
** @param log_file Path to the log file
** @param log Enable or disable logging
** @param servers Array of vhosts
** @daemon option for the daemon (START, STOP, RESTART)
*/
struct config
{
    char *pid_file;
    char *log_file;
    bool log;

    struct server_config *servers;
    enum daemon daemon;
};

/*
** @brief Vhost configuration structure
**
** @param server_name Server name
** @param port Port to listen on
** @param ip IP address
** @param root_dir Root directory to serve
** @param default_file Default file to serve
*/
struct server_config
{
    struct string *server_name;
    char *port;
    char *ip;
    char *root_dir;
    char *default_file;
};

/*
** @brief Parse the given argv argument and returns a filled struct
**
** @param argc Number of arguments
** @param argv A list of char * containing all the arguments
*/
struct config *parse_configuration(int argc, char *argv[]);

/*
** @brief Free the config struct
**
** @param config The config struct to free
*/
void config_destroy(struct config *config);

#endif /* !CONFIG_H */
