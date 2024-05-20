#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <openssl/rand.h>

#include "helpers.h"
#include "error_handling.h"

static void stringSearch_computeLps(const char* pattern, int m, int* lps) 
{
    int len = 0;
    lps[0] = 0; // lps[0] is always 0

    int i = 1;
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
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
void checkDataSizes() {
    if (sizeof(double) != 8
        || sizeof(int) != 4
        || sizeof(long long) != 8) {
        fprintf(stderr, "ERROR: Unexpected data sizes for serialization, exiting...\n");
        exit(1);
    }
}

void printBufferInHex(char *data, int size)
{
    for (size_t i = 0; i < size; ++i) {
        fprintf(stderr, "%02X ", data[i]);
    }
    fprintf(stderr, "\n");
}
/*
 *  Complicated KMP string search algo, don't bother touching
 */
int stringSearch(const char* text, const char* pattern, int maxLength) 
{
    int textLength = strnlen(text, maxLength);
    int patLength  = strnlen(pattern, maxLength);
    int* lps       = malloc(sizeof(*lps) * patLength);
    if (!lps) {
        exit(1);
    }

    stringSearch_computeLps(pattern, patLength, lps);

    int i = 0, j = 0;
    while (i < textLength && i < maxLength) {
        if (pattern[j] == text[i]) {
            j++;
            i++;
        }
        if (j == patLength) {
            free(lps);
            return i - j;
        } else if (i < textLength && pattern[j] != text[i]) {
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
bool isEmptyString(const char *string)
{
    return string[0] == '\0';    
}
int charSearch(const char *restrict text, char c, int bufLen)
{
    int i = 0;
    while (text[i] != c && i < bufLen) {
        i++;
    }
    if (i == bufLen) {
        return -1;
    }
    return i;
}

long long int getRandomInt()
{
    long long     randomInteger                      = 0;
    unsigned char randomBytes[sizeof(long long int)] = { 0 };

    if (RAND_bytes(randomBytes, sizeof(randomBytes)) != 1) {
        fprintf(stderr, "Error generating random bytes.\n");
        return 0;
    }
    for (int i = 0; i < sizeof(long long int); ++i) {
        randomInteger = (randomInteger << 8) | randomBytes[i];
    }
    return randomInteger;
}

void cap(int *intToCap, int maxValue)
{
    *intToCap = *intToCap > maxValue ? maxValue : *intToCap;
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
unsigned int hashDataSimple(const char *data, size_t data_len) 
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
 * as the inBuffer.
 *
 * Here
 *  V
 *  firstElement=data&secondElement=data.....
 *
 */
void parseHTMLForm(const char * inBuffer, struct HTMLForm *outBuffer, ssize_t inBufferLen)
{
    // TODO: Make ABSOLUTELY sure this doesn't overflow
    int i = 0;
    int fieldLen = 0;
    while (inBuffer[i] != '\0' 
           && i < inBufferLen 
           && outBuffer->fieldCount < HTMLFORM_MAX_FIELDS) 
    {
        int nextField = 0;
        fieldLen = 0;
        nextField = charSearch(&inBuffer[i], '=', inBufferLen - i + 1 );   
        if ( nextField < 0 ) {
            // no more form fields found...
            break;
        }
        i        += nextField + 1;
        fieldLen = charSearch(&inBuffer[i], '&', inBufferLen - i);
        if ( fieldLen < 0 ) {
            // No more '&' found, hitting
            // the end of the buffer...
            fieldLen = inBufferLen - i;
        }

        cap(&fieldLen, HTMLFORM_FIELD_MAX_LEN);
        memcpy(outBuffer->fields[outBuffer->fieldCount], &inBuffer[i], fieldLen);
        outBuffer->fieldCount++;
    }
}

int lockObject(pthread_mutex_t *lock)
{
    if (lock == NULL) {
        return -1;
    }
    pthread_mutex_lock(lock);
    return 0;
}
