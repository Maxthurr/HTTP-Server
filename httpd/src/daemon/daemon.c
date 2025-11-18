#include "daemon.h"

#include <stdio.h>
#include <unistd.h>

#include "../config/config.h"

static int add_pid(const struct config *config, pid_t pid)
{
    // Open file in append mode
    FILE *pid_file = fopen(config->pid_file, "a");
    if (!pid_file)
        return 1;

    fprintf(pid_file, "%d\n", pid);
    fclose(pid_file);
    return 0;
}

int start_daemon(const struct config *config)
{
    printf("Starting daemon with PID file: %s\n", config->pid_file);

    pid_t pid = fork();
    if (pid < 0)
        return 1;
    if (pid > 0)
        _exit(0); // Terminate parent process

    // Child process
    if (add_pid(config, getpid()) != 0)
        return 1;

    return 0;
}
