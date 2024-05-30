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

struct MemoryMappedFile {
    char  name[MAX_FILENAME_LEN];
    char *data;
    int   size;
};

static struct MemoryMappedFile memoryMapBuffer[MAX_FILE_COUNT] = { 0 };
static int                     memoryMapBufferSize             = 0;          

static pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * TODO:
 * We can swap this out with a
 * hash map implementation
 * at some point
 */
static inline int 
searchMmapBuffer(const char *dir, char **buffer)
{
    int foundInCache = -1;
    *buffer = NULL;
    for (int i = 0; i < memoryMapBufferSize; i++) {
        foundInCache = stringSearch(memoryMapBuffer[i].name, 
                                    dir, 
                                    MAX_FILENAME_LEN);
        if (foundInCache == 0) {
            *buffer = memoryMapBuffer[i].data;
            return memoryMapBuffer[i].size;
        }
    }
    return -1;
}
/*
 * Get file data from a directory into
 * a buffer. The files are memory-mapped
 * and cached.
 * Return the buffer from the cache if the
 * string is found there, otherwise map the file.
 */
int getFileData(const char *dir, char **buffer)
{
    int ret = 0;
    pthread_mutex_lock(&fileMutex);
    ret = searchMmapBuffer(dir, buffer);
    if (*buffer != NULL) {
        pthread_mutex_unlock(&fileMutex);
        return ret;
    }
    if (memoryMapBufferSize >= MAX_FILE_COUNT) {
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

/*
 * TODO: rewrite this spaghetti
 */
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
              snprintf(&outArray[i * MAX_FILENAME_LEN], 
              MAX_DIRNAME_LEN + MAX_FILENAME_LEN-3, 
              "./%s", 
              dir->d_name);
              i++;
          }
        }
        closedir(d);
    }
    */
    // TODO: make this a function instead of copy pasting
    d = opendir("./src");
    if (d) {
        while ((dir = readdir(d)) != NULL && i < MAX_FILE_COUNT) {  
            if (dir->d_type == DT_REG)
            {
                snprintf(&outArray[i * MAX_FILENAME_LEN], 
                         MAX_DIRNAME_LEN + MAX_FILENAME_LEN-8, 
                         "./src/%s", 
                         dir->d_name);
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
                // This produceses a potential overflow error, but it's
                // okay as long as I don't personally cause an overflow
                // with long filenames
                snprintf(&outArray[i * MAX_FILENAME_LEN], 
                         MAX_DIRNAME_LEN + MAX_FILENAME_LEN - 10, 
                         "./images/%s", 
                         dir->d_name);
                i++;
            }
        }
    }
    closedir(d);
    d = opendir("./src/rendering");
    if (d) {
        while ((dir = readdir(d)) != NULL && i < MAX_FILE_COUNT) {  
            if (dir->d_type == DT_REG)
            {
                snprintf(&outArray[i * MAX_FILENAME_LEN], 
                         MAX_DIRNAME_LEN + MAX_FILENAME_LEN-17, 
                         "./src/rendering/%s", 
                         dir->d_name);
                i++;
            }
        }
    }
    closedir(d);
    return i;
}
