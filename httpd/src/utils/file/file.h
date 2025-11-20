#ifndef FILE_H
#define FILE_H

#include <sys/types.h>

#include "../../config/config.h"
#include "../string/string.h"

off_t get_file_length(const char *path);
struct string *get_fullname(const struct config *config,
                            struct string *filename);

#endif /* ! FILE_H */
