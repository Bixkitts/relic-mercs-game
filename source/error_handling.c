#include <stdio.h>

#include "error_handling.h"

static const char errStrings[BB_ERR_COUNT][BB_ERROR_STRLEN] = {
    "\nmalloc error\n",
    "\ncalloc error\n"
};

void printError(ErrorString string)
{
    fprintf(stderr, "%s", errStrings[string]);
}
