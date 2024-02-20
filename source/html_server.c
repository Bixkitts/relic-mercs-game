#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "error_handling.h"
#include "html_server.h"

void sendForbiddenPacket(Host remotehost)
{
    char *forbidden =
        "HTTP/1.1 301 Moved Permanently\n"
        "Location: https://http.cat/403\n"
        "Content-Type: text/html\n"
        "Content-Length: 0\n\n";
    sendDataTCP(forbidden, strlen(forbidden), remotehost);
    return;
}
int getFileData(const char *dir, char **buffer)
{
    // Open the HTML file
    int fd = open(dir, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get the size of the file
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the buffer to hold the file content
    *buffer = (char *)malloc(file_stat.st_size + 1); // +1 for null terminator
    if (*buffer == NULL) {
        printError(BB_ERR_MALLOC);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Read the file content into the buffer
    ssize_t bytes_read = read(fd, *buffer, file_stat.st_size);
    if (bytes_read == -1) {
        perror("Error reading file");
        close(fd);
        free(*buffer);
        exit(EXIT_FAILURE);
    }

    // Close the file
    close(fd);
    return bytes_read;
}
