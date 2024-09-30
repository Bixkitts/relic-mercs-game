#ifndef BB_FILEHANDLING
#define BB_FILEHANDLING

#define MAX_DIRNAME_LEN  64
#define MAX_FILENAME_LEN 256
#define MAX_FILE_COUNT   256
// Returns amount of bytes read
// -1 on error
int get_file_data(const char *dir, char **buffer);
int list_files(char *out_array);
#endif
