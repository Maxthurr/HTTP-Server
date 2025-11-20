#ifndef SERVER_H
#define SERVER_H

#include "../config/config.h"

int start_server(struct config *config);
void stop_server(int cfd, int server_fd, struct config *config);
int run_server(int sfd, struct config *config);

#endif /* ! SERVER_H */
