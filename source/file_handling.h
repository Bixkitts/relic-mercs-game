#ifndef BB_FILEHANDLING
#define BB_FILEHANDLING

// Returns amount of bytes read
// -1 on error
int getFileData  (const char *dir, 
                  char **buffer);

#endif
