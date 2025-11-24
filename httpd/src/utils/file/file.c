#include "file.h"

#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

off_t get_file_length(const char *path)
{
    struct stat buffer;
    if (stat(path, &buffer) != 0)
        return -1;

    return buffer.st_size;
}
