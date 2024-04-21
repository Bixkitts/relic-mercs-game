#ifndef BB_HELPERS
#define BB_HELPERS

#include <stdio.h>
#include <pthread.h>

#define HTMLFORM_FIELD_MAX_LEN 64
#define HTMLFORM_MAX_FIELDS    64

#define ASCII_TO_INT           48

#ifdef DEBUG
#define DEBUG_TEMP    // We'll use this to put in temporary testing stuff.
                      // Preferably don't commit things with DEBUG_TEMP.
                      // TODO: Find a way to automate this.
#endif

struct HTMLForm {
   char fields[HTMLFORM_MAX_FIELDS][HTMLFORM_FIELD_MAX_LEN];
   int  fieldCount;
};
/* Helper functions */

// returns the index of the first occurence of the pattern or -1 on failure.
// Max length should be the size of the buffer we're searching
int  stringSearch     (const char* text, const char* pattern, int maxLength);

// returns the index at which c is found or -1
int  charSearch       (const char *restrict text, char c, int bufLen);

void capInt           (int *intToCap, int maxValue);

void printBufferInHex (char *data, int size);
void parseHTMLForm    (const char * inBuffer, 
                       struct HTMLForm *outBuffer, 
                       ssize_t inBufferLen);
void checkDataSizes   ();

long long int getRandomInt();

#endif
