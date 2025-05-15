#ifndef BB_HELPERS
#define BB_HELPERS

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#define HTMLFORM_FIELD_MAX_LEN 64
#define HTMLFORM_MAX_FIELDS    64

#define ASCII_TO_INT           48

#ifdef DEBUG
#define DEBUG_TEMP // We'll use this to put in temporary testing stuff.
                   // Preferably don't commit things with DEBUG_TEMP.
                   // TODO: Find a way to automate this.
#endif

struct html_form {
    char fields[HTMLFORM_MAX_FIELDS][HTMLFORM_FIELD_MAX_LEN];
    int field_count;
};
/* Helper functions */

// returns the index of the first occurence of the pattern or -1 on failure.
// Max length should be the size of the buffer we're searching
int string_search(const char *text, const char *pattern, int max_length);

// returns the index at which c is found or -1
int char_search(const char *restrict text, char c, size_t buf_len);

void cap(int *int_to_cap, int max_value);

void print_buffer_in_hex(char *data, size_t size);
void parse_html_form(const char *in_buffer,
                     struct html_form *out_buffer,
                     ssize_t in_buffer_len);
void check_data_sizes();
bool is_empty_string(const char *string);

double clamp(double x, double min, double max);

// random numbers
long long int get_random_int();
float get_random_float(float min, float max);
double get_random_double(double min, double max);

#endif
