#include <stdio.h>

#include "error_handling.h"

static const char err_strings[BB_ERR_COUNT][BB_ERROR_STRLEN] =
    {"\nmalloc error\n", "\ncalloc error\n", "\nfile not found\n"};

void print_error(enum error_string string)
{
    fprintf(stderr, "%s", err_strings[string]);
}
