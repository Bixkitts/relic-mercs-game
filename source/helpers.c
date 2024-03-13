#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <limits.h>
#include <openssl/rand.h>

#include "helpers.h"

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

int getMutexIndex(const void *object, unsigned long size, int mutexCount)
{
    return ((unsigned long)object / size) % mutexCount;
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
    int* lps       = (int*)malloc(sizeof(int) * patLength);
    if (lps == NULL) {
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

int charSearch(char* text, char c, int bufLen)
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

void capInt(int *intToCap, int maxValue)
{
    *intToCap = *intToCap > maxValue ? maxValue : *intToCap;
}

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
