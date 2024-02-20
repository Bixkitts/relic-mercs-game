#ifndef BB_RELIC_ERROR_HANDLING
#define BB_RELIC_ERROR_HANDLING

#define BB_ERROR_STRLEN 64

typedef enum {
    BB_ERR_MALLOC,
    BB_ERR_CALLOC,
    BB_ERR_COUNT
} ErrorString;

void printError(ErrorString string);

#endif
