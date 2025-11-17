#include "string.h"

#include <stdlib.h>
#include <string.h>

struct string *string_create(const char *str, size_t size)
{
    struct string *string = malloc(sizeof(struct string));
    string->data = malloc(size * sizeof(char));
    memcpy(string->data, str, size);
    string->size = size;

    return string;
}

int string_compare_n_str(const struct string *str1, const char *str2, size_t n)
{
    return memcmp(str1->data, str2, n);
}

void string_concat_str(struct string *str, const char *to_concat, size_t size)
{
    if (!size)
        return;

    str->data = realloc(str->data, (str->size + size) * sizeof(char));
    memcpy(str->data + size, to_concat, size);
}

void string_destroy(struct string *str)
{
    free(str->data);
    free(str);
}
