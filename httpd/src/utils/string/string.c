#include "string.h"

#include <ctype.h>
#include <stdbool.h>
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
    memcpy(str->data + str->size, to_concat, size);

    str->size += size;
}

bool string_n_casecmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        char c1 = tolower(s1[i]);
        char c2 = tolower(s2[i]);
        if (c1 != c2)
            return false;
    }
    return true;
}

void string_destroy(struct string *str)
{
    if (!str)
        return;

    free(str->data);
    free(str);
}
