#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 

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

int listFiles(char *outArray)
{
    DIR *d = NULL;
    int  i = 0;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL && i < MAX_FILE_COUNT) {  
          if (dir->d_type == DT_REG)
          {
              snprintf(&outArray[i * MAX_FILENAME_LEN], MAX_FILENAME_LEN-3, "./%s", dir->d_name);
              i++;
          }
        }
        closedir(d);
    }
    d = opendir("./src");
    if (d) {
        while ((dir = readdir(d)) != NULL && i < MAX_FILE_COUNT) {  
          if (dir->d_type == DT_REG)
          {
              snprintf(&outArray[i * MAX_FILENAME_LEN], MAX_FILENAME_LEN-8, "./src/%s", dir->d_name);
              i++;
          }
        }
        closedir(d);
    }
    d = opendir("./images");
    if (d) {
        while ((dir = readdir(d)) != NULL && i < MAX_FILE_COUNT) {  
          if (dir->d_type == DT_REG)
          {
              snprintf(&outArray[i * MAX_FILENAME_LEN], MAX_FILENAME_LEN-10, "./images/%s", dir->d_name);
              i++;
          }
        }
    }
    closedir(d);
    return i;
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
