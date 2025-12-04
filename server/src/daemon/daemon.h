#ifndef DAEMON_H
#define DAEMON_H

#include <stdbool.h>

#include "../config/config.h"

int start_daemon(struct config *config);
int stop_daemon(struct config *config);
int restart_daemon(struct config *config);

#endif /* ! DAEMON_H */
