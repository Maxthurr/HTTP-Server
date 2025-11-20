#include "file.h"

#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../config/config.h"
#include "../string/string.h"

off_t get_file_length(const char *path)
{
    struct stat buffer;
    if (stat(path, &buffer) != 0)
        return -1;

    return buffer.st_size;
}

struct string *get_fullname(const struct config *config,
                            struct string *filename)
{
    struct string *fullpath = string_create(config->servers->root_dir,
                                            strlen(config->servers->root_dir));

    string_concat_str(fullpath, filename->data,
                      filename->size - 1); // Exclude null byte

    if (fullpath->size > 0 && fullpath->data[fullpath->size - 1] == '/')
    {
        string_concat_str(fullpath, config->servers->default_file,
                          strlen(config->servers->default_file));
    }

    return fullpath;
}
