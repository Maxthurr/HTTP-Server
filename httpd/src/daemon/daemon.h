#ifndef DAEMON_H
#define DAEMON_H

#include <stdbool.h>

#include "../config/config.h"

int start_daemon(const struct config *config);
int stop_daemon(const struct config *config);
int restart_daemon(const struct config *config);

#endif /* ! DAEMON_H */
