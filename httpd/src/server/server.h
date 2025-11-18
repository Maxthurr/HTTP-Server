#ifndef SERVER_H
#define SERVER_H

#include "../config/config.h"

int start_server(const struct server_config *config);
void stop_server(int server_fd);
int accept_connection(int sfd);

#endif /* ! SERVER_H */
