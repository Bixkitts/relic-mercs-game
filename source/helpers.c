#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
