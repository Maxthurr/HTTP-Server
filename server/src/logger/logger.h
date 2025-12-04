#ifndef LOGGER_H
#define LOGGER_H

#include "../config/config.h"
#include "../http/http.h"

int logger_init(const struct config *config);
void logger_log(const struct config *config, const char *message);
void logger_request(const struct config *config,
                    const struct request_header *request,
                    const struct string *client_ip);
void logger_response(const struct config *config,
                     const struct request_header *request,
                     const struct string *client_ip);
void logger_error(const struct config *config, const char *source,
                  const char *message);
void logger_destroy(void);

#endif /* ! LOGGER_H */
