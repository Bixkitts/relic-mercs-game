#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_handling.h"
#include "helpers.h"

struct memory_mapped_file {
    char name[MAX_FILENAME_LEN];
    char *data;
    int size;
};

struct mem_mapped_file_buffer {
    struct memory_mapped_file files[MAX_FILE_COUNT];
    int length;
    bool initialized;
    pthread_mutex_t threadlock;
};

static struct mem_mapped_file_buffer global_file_table = { 0 };

static size_t list_files_in_directory(const char *directory_name,
                                      size_t directory_name_len,
                                      char *out_array);

/*
 * TODO:
 * We can swap this out with a
 * hash map implementation
 * at some point.
 * Returns -1 on failure.
 */
static inline int search_mmap_buffer(const char *dir, char *out_buffer)
{
    assert(!out_buffer);
    for (int i = 0; i < global_file_table.length; i++) {
        int found_in_cache = string_search(global_file_table.files[i].name,
                                           dir,
                                           MAX_FILENAME_LEN);
        if (!found_in_cache) {
            out_buffer = global_file_table.files[i].data;
            return global_file_table.files[i].size;
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
size_t get_file_data(const char *dir, char **out_buffer)
{
    assert(out_buffer && !*out_buffer);
    if (!global_file_table.initialized) {
       pthread_mutex_init(&global_file_table.threadlock, NULL); 
    }
    pthread_mutex_lock(&global_file_table.threadlock);
    long ret = search_mmap_buffer(dir, *out_buffer);
    if (*out_buffer) {
        goto cleanup_threadlock;
    }
    if (global_file_table.length >= MAX_FILE_COUNT) {
        fprintf(stderr, "\nError, too many files open.\n");
        goto cleanup_threadlock;
    }
    int fd = open(dir, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        goto cleanup_threadlock;
    }
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        goto cleanup_file_handle;
    }
    ret = file_stat.st_size;
    global_file_table.files[global_file_table.length].data =
        mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    strncpy(global_file_table.files[global_file_table.length].name,
            dir,
            MAX_FILENAME_LEN);
    global_file_table.files[global_file_table.length].size = file_stat.st_size;

    *out_buffer = global_file_table.files[global_file_table.length].data;
    if (MAP_FAILED == *out_buffer) {
        perror("Error mapping file into memory");
        goto cleanup_file_handle;
    }
    global_file_table.length++;

cleanup_file_handle:
    close(fd);
cleanup_threadlock:
    pthread_mutex_unlock(&global_file_table.threadlock);
    return ret;
}

/*
 * TODO: rewrite this spaghetti
 */
size_t list_files(char *out_array)
{
    size_t file_count = 0;

    const char *src_rendering_dir = "./src/rendering/";
    file_count += list_files_in_directory(src_rendering_dir, strlen(src_rendering_dir), &out_array[file_count * MAX_FILENAME_LEN]);

    const char *src_dir = "./src/";
    file_count += list_files_in_directory(src_dir, strlen(src_dir), &out_array[file_count * MAX_FILENAME_LEN]);

    const char *images_dir = "./images/";
    file_count += list_files_in_directory(images_dir, strlen(images_dir), &out_array[file_count * MAX_FILENAME_LEN]);

    return file_count;
}

/*
 * Writes the names of one file after the other.
 * Returns the number of chars written.
 */
static size_t list_files_in_directory(const char *directory_name,
                                      size_t directory_name_len,
                                      char *out_array)
{
    assert(strlen(directory_name) == directory_name_len);
    struct dirent *dir        = NULL;
    DIR           *d          = opendir(directory_name);
    size_t         file_count = 0;

    if (!d) {
        return file_count;
    }
    while ((dir = readdir(d)) && file_count < MAX_FILE_COUNT) {
        if (dir->d_type == DT_REG) {
            snprintf(&out_array[file_count * MAX_FILENAME_LEN],
                     MAX_DIRNAME_LEN + MAX_FILENAME_LEN - directory_name_len,
                     "%s%s",
                     directory_name,
                     dir->d_name);
            file_count++;
        }
    }
    closedir(d);
    return file_count;
}
