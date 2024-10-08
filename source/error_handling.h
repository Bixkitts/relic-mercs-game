#ifndef BB_RELIC_ERROR_HANDLING
#define BB_RELIC_ERROR_HANDLING

#define BB_ERROR_STRLEN 64

enum error_string {
    BB_ERR_MALLOC,
    BB_ERR_CALLOC,
    BB_ERR_FILE_NOT_FOUND,
    BB_ERR_COUNT
};

void print_error(enum error_string string);

#endif
