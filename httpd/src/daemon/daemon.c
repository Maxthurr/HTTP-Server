#include "daemon.h"

#include <stdio.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../config/config.h"

static int add_pid(const struct config *config, pid_t pid)
{
    // Open file in append mode
    FILE *pid_file = fopen(config->pid_file, "a");
    if (!pid_file)
        return 1;

    // Write PID to file
    fprintf(pid_file, "%d", pid);
    fclose(pid_file);
    return 0;
}

static bool pid_exists(const struct config *config)
{
    FILE *pid_file = fopen(config->pid_file, "r");
    if (!pid_file)
        return false;

    pid_t existing_pid;
    while (fscanf(pid_file, "%d", &existing_pid) == 1)
    {
        // Send signal 0 to check if process exists
        if (kill(existing_pid, 0) == 0)
        {
            fclose(pid_file);
            return true;
        }
    }

    fclose(pid_file);
    return false;
}

int start_daemon(struct config *config)
{
    if (pid_exists(config))
        return 1;

    pid_t pid = fork();
    if (pid < 0)
        return 1;
    if (pid > 0)
        _exit(0); // Terminate parent process

    // Add child PID to PID file
    if (add_pid(config, getpid()) != 0)
        return 1;

    return 0;
}

int stop_daemon(struct config *config)
{
    FILE *pid_file = fopen(config->pid_file, "r");
    if (!pid_file)
        return 0;

    pid_t existing_pid;
    while (fscanf(pid_file, "%d", &existing_pid) == 1)
    {
        // Kill the process
        if (kill(existing_pid, SIGINT) == 0)
        {
            // Wait for the process to properly terminate
            waitpid(existing_pid, NULL, 0);
            break;
        }
    }

    // Close PID file and delete it
    fclose(pid_file);
    remove(config->pid_file);

    return 0;
}

int restart_daemon(struct config *config)
{
    stop_daemon(config);
    return start_daemon(config);
}
