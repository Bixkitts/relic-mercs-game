#include <dirent.h>
#include <openssl/rand.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handling.h"
#include "helpers.h"

static void string_search_compute_lps(const char *pattern, int m, int *lps)
{
    int len = 0;
    lps[0]  = 0; // lps[0] is always 0

    int i = 1;
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else {
            if (len != 0) {
                len = lps[len - 1];
            }
            else {
                lps[i] = 0;
                i++;
            }
        }
    }
}

/*
 * This function makes sure the data types we use are the
 * correct length for interpreting incoming websocket packets.
 * This is ONLY to be run in debug mode and exists because I don't
 * want to consult the C standard for guarenteed type sizes,
 * or make/find custom fixed length types for all data coming in.
 */
void check_data_sizes()
{
    if (sizeof(double) != 8 || sizeof(int) != 4 || sizeof(long long) != 8) {
        fprintf(stderr,
                "ERROR: Unexpected data sizes for serialization, exiting...\n");
        exit(1);
    }
}

void print_buffer_in_hex(char *data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "%02X ", data[i]);
    }
    fprintf(stderr, "\n");
}
/*
 *  Complicated KMP string search algo, don't bother touching
 */
int string_search(const char *text, const char *pattern, int max_length)
{
    int text_length = strnlen(text, max_length);
    int pat_length  = strnlen(pattern, max_length);
    int *lps        = malloc(sizeof(*lps) * pat_length);
    if (!lps) {
        exit(1);
    }

    string_search_compute_lps(pattern, pat_length, lps);

    int i = 0, j = 0;
    while (i < text_length && i < max_length) {
        if (pattern[j] == text[i]) {
            j++;
            i++;
        }
        if (j == pat_length) {
            free(lps);
            return i - j;
        }
        else if (i < text_length && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            }
            else {
                i++;
            }
        }
    }
    free(lps);
    return -1;
}

/*
 * Assumes non-null string
 */
bool is_empty_string(const char *string)
{
    return string[0] == '\0';
}
int char_search(const char *restrict text, char c, size_t buf_len)
{
    if (buf_len >= INT_MAX) {
        return -1;
    }
    size_t i = 0;
    while (text[i] != c && i < buf_len) {
        i++;
    }
    if (i == buf_len) {
        return -1;
    }
    return i;
}

float get_random_float(float min, float max)
{
    unsigned char buffer[sizeof(float)] = {0}; // 4 bytes to store a random integer
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
        fprintf(stderr, "Error generating random bytes\n");
        exit(1);
    }
    unsigned int rand_int = 0;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        rand_int = (rand_int << 8) | buffer[i];
    }
    float normalized = rand_int / (float)UINT_MAX;
    float result     = min + normalized * (max - min);

    return result;
}

double get_random_double(double min, double max)
{
    unsigned char buffer[sizeof(double)]; // 4 bytes to store a random integer
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
        fprintf(stderr, "Error generating random bytes\n");
        exit(1);
    }
    unsigned int rand_int = 0;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        rand_int = (rand_int << 8) | buffer[i];
    }
    double normalized = rand_int / (double)UINT64_MAX;
    double result     = min + normalized * (max - min);

    return result;
}

long long int get_random_int()
{
    long long random_integer                          = 0;
    unsigned char random_bytes[sizeof(long long int)] = {0};

    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        fprintf(stderr, "Error generating random bytes.\n");
        return 0;
    }
    for (size_t i = 0; i < sizeof(long long int); i++) {
        random_integer = (random_integer << 8) | random_bytes[i];
    }
    return random_integer;
}

void cap(int *int_to_cap, int max_value)
{
    *int_to_cap = *int_to_cap > max_value ? max_value : *int_to_cap;
}

double clamp(double x, double min, double max)
{
    const double t = x < min ? min : x;
    return t > max ? max : t;
}

/*
 * TODO: this was just copy pasted from
 * Chat GPT, idek if it works, I need a proper simple
 * hash algorithm for hash tables.
 */
unsigned int hash_data_simple(const char *data, size_t data_len)
{
    unsigned int hash = 0;

    for (size_t i = 0; i < data_len; i++) {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/*
 * Expects a pointer to the
 * first element of the HTML form
 * as the in_buffer.
 *
 * Here
 *  V
 *  firstElement=data&secondElement=data.....
 *
 */
void parse_html_form(const char *in_buffer,
                     struct html_form *out_buffer,
                     ssize_t in_buffer_len)
{
    // TODO: Make ABSOLUTELY sure this doesn't overflow
    int i         = 0;
    int field_len = 0;
    while (in_buffer[i] != '\0' && i < in_buffer_len &&
           out_buffer->field_count < HTMLFORM_MAX_FIELDS) {
        int next_field = 0;
        field_len      = 0;
        next_field     = char_search(&in_buffer[i], '=', in_buffer_len - i + 1);
        if (next_field < 0) {
            // no more form fields found...
            break;
        }
        i += next_field + 1;
        field_len = char_search(&in_buffer[i], '&', in_buffer_len - i);
        if (field_len < 0) {
            // No more '&' found, hitting
            // the end of the buffer...
            field_len = in_buffer_len - i;
        }

        cap(&field_len, HTMLFORM_FIELD_MAX_LEN);
        memcpy(out_buffer->fields[out_buffer->field_count],
               &in_buffer[i],
               field_len);
        out_buffer->field_count++;
    }
}


struct char_slice slice_string(const char *string,
                               const ssize_t start_index,
                               const ssize_t string_length,
                               const ssize_t len)
{
    const struct char_slice slice =
    { &string[start_index], len};
    return slice;
}

struct char_slice slice_string_to(const char *string,
                                  const ssize_t start_index,
                                  const ssize_t string_length,
                                  const char until_this)
{
    const char *text = &string[start_index];
    const struct char_slice slice =
    { text,
      char_search(text, until_this, string_length - start_index)};
    return slice;
}
