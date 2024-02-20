/* String helper functions */

// returns the index of the first occurence of the pattern or -1 on failure.
// Max length should be the size of the buffer we're searching
int stringSearch(const char* text, const char* pattern, int maxLength);

// returns the index at which c is found or -1
int charSearch  (char* text, char c, int bufLen);
