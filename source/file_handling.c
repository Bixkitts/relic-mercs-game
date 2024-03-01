#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include "error_handling.h"

static pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

int getFileData(const char *dir, char **buffer)
{
    pthread_mutex_lock(&fileMutex);
    // Open the HTML file
    int fd = open(dir, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    // Get the size of the file
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(fd);
        return -1;
    }

    // Allocate memory for the buffer to hold the file content
    *buffer = (char *)calloc(1, file_stat.st_size + 1); // +1 for null terminator
    if (*buffer == NULL) {
        printError(BB_ERR_MALLOC);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Read the file content into the buffer
    ssize_t bytes_read = read(fd, *buffer, file_stat.st_size);
    if (bytes_read == -1) {
        perror("Error reading file");
        free(*buffer);
    }

    // Close the file
    close(fd);
    pthread_mutex_unlock(&fileMutex);
    return bytes_read;
}
