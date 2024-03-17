#ifndef BB_HELPERS
#define BB_HELPERS

#include <stdio.h>

#define HTMLFORM_FIELD_MAX_LEN 64
#define HTMLFORM_MAX_FIELDS    64

typedef struct HTMLForm {
   char fields[HTMLFORM_MAX_FIELDS][HTMLFORM_FIELD_MAX_LEN];
   int  fieldCount;
} HTMLForm;
/* Helper functions */

// returns the index of the first occurence of the pattern or -1 on failure.
// Max length should be the size of the buffer we're searching
int  stringSearch     (const char* text, const char* pattern, int maxLength);

// returns the index at which c is found or -1
int  charSearch       (const char *restrict text, char c, int bufLen);

void capInt           (int *intToCap, int maxValue);

void printBufferInHex (char *data, int size);
int  getMutexIndex    (const void *object, 
                       unsigned long size, 
                       int mutexCount);
void parseHTMLForm    (const char * inBuffer, 
                       HTMLForm *outBuffer, 
                       ssize_t inBufferLen);

long long int getRandomInt();

#endif
