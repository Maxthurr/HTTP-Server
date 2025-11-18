#include "logger.h"

#include <stdio.h>
#include <time.h>

#include "../config/config.h"
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

    // Get current time in GMT
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    // Format time as Day (3 letters), DD Mon YYYY HH:MM:SS GMT
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    fprintf(log_file, "%s [%s] %s\n", time_str,
            config->servers->server_name->data, message);

    fflush(log_file);
}

void logger_destroy(void)
{
    // Ensure all logs are flushed before closing
    fflush(log_file);
    if (log_file && log_file != stdout)
        fclose(log_file);
    log_file = NULL;
}
