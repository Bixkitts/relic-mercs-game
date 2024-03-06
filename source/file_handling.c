#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <dirent.h> 

#include "error_handling.h"
#include "file_handling.h"
#include "helpers.h"

typedef struct MemoryMappedFile {
    char  name[MAX_FILENAME_LEN];
    char *data;
    int   size;
}MemoryMappedFile;

static MemoryMappedFile memoryMapBuffer[MAX_FILE_COUNT] = { 0 };
static int              memoryMapBufferSize             = 0;          

static pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Get file data from a directory into
 * a buffer. The files are memory-mapped
 * and cached.
 * Return the buffer from the cache if the
 * string is found there, otherwise map the file.
 */
int getFileData(const char *dir, char **buffer)
{
    pthread_mutex_lock(&fileMutex);
    int foundInCache = -1;
    for (int i = 0; i < memoryMapBufferSize; i++) {
        foundInCache = stringSearch(memoryMapBuffer[i].name, dir, MAX_FILENAME_LEN);
        if (foundInCache == 0) {
            *buffer = memoryMapBuffer[i].data;
            pthread_mutex_unlock(&fileMutex);
            return memoryMapBuffer[i].size;
        }
    }
    if (memoryMapBufferSize >= MAX_FILENAME_LEN) {
        fprintf(stderr, "\nError, too many files open.\n");
    }
    // The file was not found in the cache,
    // mmap it and cache it.
    int fd = open(dir, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        pthread_mutex_unlock(&fileMutex);
        return -1;
    }
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(fd);
        pthread_mutex_unlock(&fileMutex);
        return -1;
    }
    memoryMapBuffer[memoryMapBufferSize].data = 
    mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    strncpy(memoryMapBuffer[memoryMapBufferSize].name, dir, MAX_FILENAME_LEN);
    memoryMapBuffer[memoryMapBufferSize].size = file_stat.st_size;

    *buffer = memoryMapBuffer[memoryMapBufferSize].data;
    if (*buffer == MAP_FAILED) {
        perror("Error mapping file into memory");
        close(fd);
        pthread_mutex_unlock(&fileMutex);
        return -1;
    }
    memoryMapBufferSize++;


    // Close the file
    close(fd);
    pthread_mutex_unlock(&fileMutex);
    return file_stat.st_size;
}

int listFiles(char *outArray)
{
    DIR *d = NULL;
    int  i = 0;
    struct dirent *dir;
    /*
     * Files in the same dir as the executable should not be listed.
     *
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
    */
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
